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

struct PsGlobals
{
	Vec4 color;
	Vec3 playerPos;
	float range;
};

void BasicShader::Shader::Release()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShader);
	SafeRelease(layout);
}

//=================================================================================================
BasicShader::BasicShader() : deviceContext(app::render->GetDeviceContext()), vsGlobals(nullptr), psGlobals(nullptr)
	//vb(nullptr), batch(false)
{
}

//=================================================================================================
void BasicShader::OnInit()
{
	app::render->CreateShader("basic.hlsl", VDI_POS, shaderSimple.vertexShader, shaderSimple.pixelShader, shaderSimple.layout, nullptr, "VsSimple", "PsSimple");
	app::render->CreateShader("basic.hlsl", VDI_COLOR, shaderColor.vertexShader, shaderColor.pixelShader, shaderColor.layout, nullptr, "VsColor", "PsColor");
	app::render->CreateShader("basic.hlsl", VDI_POS, shaderArea.vertexShader, shaderArea.pixelShader, shaderArea.layout, nullptr, "VsArea", "PsArea");

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "BasicVsGlobals");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "BasicPsGlobals");

	meshes[(int)DebugNode::Box] = app::res_mgr->Get<Mesh>("box.qmsh");
	meshes[(int)DebugNode::Sphere] = app::res_mgr->Get<Mesh>("sphere.qmsh");
	meshes[(int)DebugNode::Capsule] = app::res_mgr->Get<Mesh>("capsule.qmsh");
	meshes[(int)DebugNode::Cylinder] = app::res_mgr->Get<Mesh>("cylinder.qmsh");
}

//=================================================================================================
void BasicShader::OnRelease()
{
	shaderSimple.Release();
	shaderColor.Release();
	shaderArea.Release();
	SafeRelease(vsGlobals);
	SafeRelease(psGlobals);
}

//=================================================================================================
/*void BasicShader::Prepare(const Camera& camera)
{
	mat_view_proj = camera.mat_view_proj;

	app::render->SetAlphaBlend(true);
	app::render->SetNoZWrite(false);
	app::render->SetNoCulling(true);
	V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(VDI_COLOR)));
	if(vb)
		V(device->SetStreamSource(0, vb, 0, sizeof(VColor)));
}

//=================================================================================================
void BasicShader::BeginBatch()
{
	assert(!batch);
	batch = true;
}

//=================================================================================================
void BasicShader::AddQuad(const Vec3(&pts)[4], const Vec4& color)
{
	verts.push_back(VColor(pts[0], color));
	verts.push_back(VColor(pts[1], color));
	verts.push_back(VColor(pts[2], color));
	verts.push_back(VColor(pts[2], color));
	verts.push_back(VColor(pts[1], color));
	verts.push_back(VColor(pts[3], color));
}

//=================================================================================================
void BasicShader::EndBatch()
{
	assert(batch);
	batch = false;

	if(verts.empty())
		return;

	if(!vb || verts.size() > vb_size)
	{
		SafeRelease(vb);
		V(device->CreateVertexBuffer(verts.size() * sizeof(VColor), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &vb, nullptr));
		vb_size = verts.size();
		V(device->SetStreamSource(0, vb, 0, sizeof(VColor)));
	}

	void* ptr;
	V(vb->Lock(0, 0, &ptr, D3DLOCK_DISCARD));
	memcpy(ptr, verts.data(), verts.size() * sizeof(VColor));
	V(vb->Unlock());

	uint passes;

	V(effect->SetTechnique(techSimple));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	V(effect->SetMatrix(hMatCombined, (const D3DXMATRIX*)&mat_view_proj));
	V(effect->CommitChanges());

	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, verts.size() / 3));

	V(effect->EndPass());
	V(effect->End());

	verts.clear();
}*/

void BasicShader::DrawDebugNodes(const vector<DebugNode*>& nodes)
{
	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_NO);
	app::render->SetRasterState(Render::RASTER_WIREFRAME);

	// setup shader
	deviceContext->VSSetShader(shaderSimple.vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(shaderSimple.pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	deviceContext->IASetInputLayout(shaderSimple.layout);

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
			V(deviceContext->Map(psGlobals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
			PsGlobals& psg = *(PsGlobals*)resource.pData;
			psg.color = node.color;
			deviceContext->Unmap(psGlobals, 0);
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

void BasicShader::PrepareArea(const Camera& camera, const Vec3& playerPos)
{
	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_READ);
	app::render->SetRasterState(Render::RASTER_NO_CULLING);

	// setup shader
	deviceContext->VSSetShader(shaderArea.vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(shaderArea.pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	deviceContext->IASetInputLayout(shaderArea.layout);

	// set matrix
	D3D11_MAPPED_SUBRESOURCE resource;
	V(deviceContext->Map(vsGlobals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VsGlobals& vsg = *(VsGlobals*)resource.pData;
	vsg.matCombined = camera.mat_view_proj.Transpose();
	deviceContext->Unmap(vsGlobals, 0);

	this->playerPos = playerPos;
}

void BasicShader::SetAreaParams(Color color, float range)
{
	D3D11_MAPPED_SUBRESOURCE resource;
	V(deviceContext->Map(psGlobals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PsGlobals& psg = *(PsGlobals*)resource.pData;
	psg.color = color;
	psg.playerPos = playerPos;
	psg.range = range;
	deviceContext->Unmap(psGlobals, 0);
}

void BasicShader::DrawArea()
{

}
