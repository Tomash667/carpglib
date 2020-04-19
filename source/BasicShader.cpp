#include "Pch.h"
#include "BasicShader.h"

#include "Camera.h"
#include "DebugNode.h"
#include "DirectX.h"
#include "Mesh.h"
#include "Render.h"
#include "ResourceManager.h"

struct VsGlobals
{
	Matrix matCombined;
};

struct PsGlobalsMesh
{
	Vec4 color;
};

struct PsGlobalsColor
{
	Vec3 playerPos;
	float range;
	float falloff;
};

void BasicShader::Shader::Release()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShader);
	SafeRelease(layout);
}

//=================================================================================================
BasicShader::BasicShader() : deviceContext(app::render->GetDeviceContext()), vsGlobals(nullptr), psGlobalsMesh(nullptr), psGlobalsColor(nullptr),
vb(nullptr), ib(nullptr), vbSize(0), ibSize(0)
{
}

//=================================================================================================
void BasicShader::OnInit()
{
	app::render->CreateShader("basic.hlsl", VDI_POS, shaderMesh.vertexShader, shaderMesh.pixelShader, shaderMesh.layout, nullptr, "VsMesh", "PsMesh");
	app::render->CreateShader("basic.hlsl", VDI_COLOR, shaderColor.vertexShader, shaderColor.pixelShader, shaderColor.layout, nullptr, "VsColor", "PsColor");

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "BasicVsGlobals");
	psGlobalsMesh = app::render->CreateConstantBuffer(sizeof(PsGlobalsMesh), "BasicPsGlobalsMesh");
	psGlobalsColor = app::render->CreateConstantBuffer(sizeof(PsGlobalsColor), "BasicPsGlobalsColor");

	meshes[(int)DebugNode::Box] = app::res_mgr->Get<Mesh>("box.qmsh");
	meshes[(int)DebugNode::Sphere] = app::res_mgr->Get<Mesh>("sphere.qmsh");
	meshes[(int)DebugNode::Capsule] = app::res_mgr->Get<Mesh>("capsule.qmsh");
	meshes[(int)DebugNode::Cylinder] = app::res_mgr->Get<Mesh>("cylinder.qmsh");
}

//=================================================================================================
void BasicShader::OnRelease()
{
	shaderMesh.Release();
	shaderColor.Release();
	SafeRelease(vsGlobals);
	SafeRelease(psGlobalsMesh);
	SafeRelease(psGlobalsColor);
	SafeRelease(vb);
	SafeRelease(ib);
	vbSize = 0;
	ibSize = 0;
}

//=================================================================================================
void BasicShader::DrawDebugNodes(const vector<DebugNode*>& nodes)
{
	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_NO);
	app::render->SetRasterState(Render::RASTER_WIREFRAME);

	// setup shader
	deviceContext->VSSetShader(shaderMesh.vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(shaderMesh.pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobalsMesh);
	deviceContext->IASetInputLayout(shaderMesh.layout);

	Color prevColor = Color::None;
	DebugNode::Mesh prevMesh = DebugNode::None;

	for(vector<DebugNode*>::const_iterator it = nodes.begin(), end = nodes.end(); it != end; ++it)
	{
		const DebugNode& node = **it;

		// set color
		if(node.color != prevColor)
		{
			prevColor = node.color;
			D3D11_MAPPED_SUBRESOURCE resource;
			V(deviceContext->Map(psGlobalsMesh, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
			PsGlobalsMesh& psg = *(PsGlobalsMesh*)resource.pData;
			psg.color = node.color;
			deviceContext->Unmap(psGlobalsMesh, 0);
		}

		// set matrix
		D3D11_MAPPED_SUBRESOURCE resource;
		V(deviceContext->Map(vsGlobals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
		VsGlobals& vsg = *(VsGlobals*)resource.pData;
		vsg.matCombined = node.mat.Transpose();
		deviceContext->Unmap(vsGlobals, 0);

		if(node.mesh == DebugNode::TriMesh)
		{
			node.trimesh->Build();

			uint stride = sizeof(VPos), offset = 0;
			deviceContext->IASetVertexBuffers(0, 1, &node.trimesh->vb, &stride, &offset);
			deviceContext->IASetIndexBuffer(node.trimesh->ib, DXGI_FORMAT_R16_UINT, 0);

			deviceContext->DrawIndexed(node.trimesh->indices.size(), 0, 0);
		}
		else
		{
			Mesh& mesh = *meshes[node.mesh];
			if(node.mesh != prevMesh)
			{
				uint stride = sizeof(VPos), offset = 0;
				deviceContext->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
				deviceContext->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R16_UINT, 0);
			}

			for(Mesh::Submesh& sub : mesh.subs)
				deviceContext->DrawIndexed(sub.tris * 3, sub.first * 3, 0);
		}
	}
}

//=================================================================================================
void BasicShader::Prepare(const Camera& camera)
{
	app::render->SetBlendState(Render::BLEND_ADD);
	app::render->SetDepthState(Render::DEPTH_READ);
	app::render->SetRasterState(Render::RASTER_NO_CULLING);

	// setup shader
	deviceContext->VSSetShader(shaderColor.vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(shaderColor.pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobalsColor);
	uint size = sizeof(VColor), offset = 0;
	deviceContext->IASetInputLayout(shaderColor.layout);
	deviceContext->IASetVertexBuffers(0, 1, &vb, &size, &offset);
	deviceContext->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);

	// set matrix
	D3D11_MAPPED_SUBRESOURCE resource;
	V(deviceContext->Map(vsGlobals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VsGlobals& vsg = *(VsGlobals*)resource.pData;
	vsg.matCombined = camera.mat_view_proj.Transpose();
	deviceContext->Unmap(vsGlobals, 0);
}

//=================================================================================================
void BasicShader::SetAreaParams(const Vec3& playerPos, float range, float falloff)
{
	assert(vertices.empty());

	D3D11_MAPPED_SUBRESOURCE resource;
	V(deviceContext->Map(psGlobalsColor, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PsGlobalsColor& psg = *(PsGlobalsColor*)resource.pData;
	psg.playerPos = playerPos;
	psg.range = range;
	psg.falloff = falloff;
	deviceContext->Unmap(psGlobalsColor, 0);
}

//=================================================================================================
void BasicShader::ReserveVertexBuffer(uint vertexCount)
{
	if(vertexCount <= vbSize)
		return;

	SafeRelease(vb);
	vbSize = vertexCount;

	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(VColor) * vertexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	V(app::render->GetDevice()->CreateBuffer(&desc, nullptr, &vb));
	SetDebugName(vb, "BasicVb");

	uint size = sizeof(VColor), offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &vb, &size, &offset);
}

//=================================================================================================
void BasicShader::ReserveIndexBuffer(uint indexCount)
{
	if(indexCount <= ibSize)
		return;

	SafeRelease(ib);
	ibSize = indexCount;

	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(word) * indexCount;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	V(app::render->GetDevice()->CreateBuffer(&desc, nullptr, &ib));
	SetDebugName(ib, "BasicIb");

	deviceContext->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
}

//=================================================================================================
void BasicShader::DrawQuad(const Vec3(&pts)[4], Color color)
{
	Vec4 col = color;
	uint offset = vertices.size();
	for(int i=0; i<4; ++i)
		vertices.push_back(VColor(pts[i], col));
	indices.push_back(offset + 0);
	indices.push_back(offset + 1);
	indices.push_back(offset + 2);
	indices.push_back(offset + 2);
	indices.push_back(offset + 1);
	indices.push_back(offset + 3);
}

//=================================================================================================
void BasicShader::DrawArea(const vector<Vec3>& vertices, const vector<word>& indices, Color color)
{
	Vec4 col = color;
	uint offset = this->vertices.size();
	for(const Vec3& pos : vertices)
		this->vertices.push_back(VColor(pos, col));
	for(word idx : indices)
		this->indices.push_back(idx + offset);
}

//=================================================================================================
void BasicShader::Draw()
{
	if(vertices.empty())
		return;

	// copy vertices
	{
		ReserveVertexBuffer(vertices.size());
		ResourceLock lock(vb, D3D11_MAP_WRITE_DISCARD);
		memcpy(lock.Get(), vertices.data(), sizeof(VColor) * vertices.size());
	}

	// copy indices
	{
		ReserveIndexBuffer(indices.size());
		ResourceLock lock(ib, D3D11_MAP_WRITE_DISCARD);
		memcpy(lock.Get(), indices.data(), sizeof(word) * indices.size());
	}

	// draw
	deviceContext->DrawIndexed(indices.size(), 0, 0);

	vertices.clear();
	indices.clear();
}
