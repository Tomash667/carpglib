#include "Pch.h"
#include "BasicShader.h"

#include "Camera.h"
#include "DebugNode.h"
#include "DirectX.h"
#include "Mesh.h"
#include "Render.h"
#include "ResourceManager.h"
#include "SimpleMesh.h"

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
	Render::ShaderParams params;
	params.name = "basic";
	params.decl = VDI_POS;
	params.vertexShader = &shaderMesh.vertexShader;
	params.pixelShader = &shaderMesh.pixelShader;
	params.layout = &shaderMesh.layout;
	params.vsEntry = "VsMesh";
	params.psEntry = "PsMesh";
	app::render->CreateShader(params);

	params.decl = VDI_COLOR;
	params.vertexShader = &shaderColor.vertexShader;
	params.pixelShader = &shaderColor.pixelShader;
	params.layout = &shaderColor.layout;
	params.vsEntry = "VsColor";
	params.psEntry = "PsColor";
	app::render->CreateShader(params);

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "BasicVsGlobals");
	psGlobalsMesh = app::render->CreateConstantBuffer(sizeof(PsGlobalsMesh), "BasicPsGlobalsMesh");
	psGlobalsColor = app::render->CreateConstantBuffer(sizeof(PsGlobalsColor), "BasicPsGlobalsColor");

	meshes[(int)MeshShape::Box] = app::resMgr->Get<Mesh>("builtin/box.qmsh");
	meshes[(int)MeshShape::Sphere] = app::resMgr->Get<Mesh>("builtin/sphere.qmsh");
	meshes[(int)MeshShape::Capsule] = app::resMgr->Get<Mesh>("builtin/capsule.qmsh");
	meshes[(int)MeshShape::Cylinder] = app::resMgr->Get<Mesh>("builtin/cylinder.qmsh");
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
void BasicShader::PrepareForShapes(const Camera& camera)
{
	matViewProj = camera.matViewProj;

	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_NO);
	app::render->SetRasterState(Render::RASTER_WIREFRAME);

	// setup shader
	deviceContext->VSSetShader(shaderMesh.vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(shaderMesh.pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobalsMesh);
	deviceContext->IASetInputLayout(shaderMesh.layout);

	prevColor = Color::None;
	prevShape = MeshShape::None;
}

//=================================================================================================
void BasicShader::DrawDebugNodes(const vector<DebugNode*>& nodes)
{
	for(vector<DebugNode*>::const_iterator it = nodes.begin(), end = nodes.end(); it != end; ++it)
	{
		const DebugNode& node = **it;

		// set color
		if(node.color != prevColor)
		{
			prevColor = node.color;
			ResourceLock lock(psGlobalsMesh);
			lock.Get<PsGlobalsMesh>()->color = node.color;
		}

		// set matrix
		{
			ResourceLock lock(vsGlobals);
			lock.Get<VsGlobals>()->matCombined = node.mat.Transpose();
		}

		if(node.shape == MeshShape::TriMesh)
		{
			node.trimesh->Build();

			uint stride = sizeof(VPos), offset = 0;
			deviceContext->IASetVertexBuffers(0, 1, &node.trimesh->vb, &stride, &offset);
			deviceContext->IASetIndexBuffer(node.trimesh->ib, DXGI_FORMAT_R16_UINT, 0);

			deviceContext->DrawIndexed(node.trimesh->indexCount, 0, 0);
		}
		else
		{
			Mesh& mesh = *meshes[(int)node.shape];
			if(node.shape != prevShape)
			{
				uint stride = sizeof(VPos), offset = 0;
				deviceContext->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
				deviceContext->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R16_UINT, 0);
			}

			for(Mesh::Submesh& sub : mesh.subs)
				deviceContext->DrawIndexed(sub.tris * 3, sub.first * 3, 0);
		}

		prevShape = node.shape;
	}
}

//=================================================================================================
void BasicShader::DrawShape(MeshShape shape, const Matrix& m, Color color)
{
	assert(shape != MeshShape::None && shape != MeshShape::TriMesh);

	// set color
	if(color != prevColor)
	{
		prevColor = color;
		ResourceLock lock(psGlobalsMesh);
		lock.Get<PsGlobalsMesh>()->color = color;
	}

	// set matrix
	{
		ResourceLock lock(vsGlobals);
		lock.Get<VsGlobals>()->matCombined = (m * matViewProj).Transpose();
	}

	// set mesh
	Mesh& mesh = *meshes[(int)shape];
	if(shape != prevShape)
	{
		uint stride = sizeof(VPos), offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
		deviceContext->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R16_UINT, 0);
		prevShape = shape;
	}

	// draw
	for(Mesh::Submesh& sub : mesh.subs)
		deviceContext->DrawIndexed(sub.tris * 3, sub.first * 3, 0);
}

//=================================================================================================
void BasicShader::Prepare(const Camera& camera)
{
	camPos = camera.from;

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
	{
		ResourceLock lock(vsGlobals);
		VsGlobals& vsg = *lock.Get<VsGlobals>();
		vsg.matCombined = camera.matViewProj.Transpose();
	}

	SetAreaParams(Vec3::Zero, 1.f, 0.f);
}

//=================================================================================================
void BasicShader::SetAreaParams(const Vec3& playerPos, float range, float falloff)
{
	assert(vertices.empty());

	ResourceLock lock(psGlobalsColor);
	PsGlobalsColor& psg = *lock.Get<PsGlobalsColor>();
	psg.playerPos = playerPos;
	psg.range = range;
	psg.falloff = falloff;
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
void BasicShader::DrawQuad(const array<Vec3, 4>& pts, Color color)
{
	Vec4 col = color;
	uint offset = vertices.size();
	for(int i = 0; i < 4; ++i)
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
void BasicShader::DrawLine(const Vec3& from, const Vec3& to, float width, Color color)
{
	width /= 2;

	Vec3 lineDir = from - to;
	Vec3 quadNormal = camPos - (to + from) / 2;
	Vec3 extrudeDir = lineDir.Cross(quadNormal).Normalize();
	Vec3 lineNormal = lineDir.Normalized() * width;

	const array<Vec3, 4> pts = {
		from + extrudeDir * width + lineNormal,
		from - extrudeDir * width + lineNormal,
		to + extrudeDir * width - lineNormal,
		to - extrudeDir * width - lineNormal
	};

	DrawQuad(pts, color);
}

//=================================================================================================
void BasicShader::DrawFrustum(const FrustumPlanes& frustum, float width, Color color)
{
	array<Vec3, 8> pts;
	frustum.GetPoints(pts);

	DrawLine(pts[FrustumPlanes::NEAR_LEFT_BOTTOM], pts[FrustumPlanes::NEAR_RIGHT_BOTTOM], width, color);
	DrawLine(pts[FrustumPlanes::NEAR_LEFT_TOP], pts[FrustumPlanes::NEAR_RIGHT_TOP], width, color);
	DrawLine(pts[FrustumPlanes::NEAR_LEFT_BOTTOM], pts[FrustumPlanes::NEAR_LEFT_TOP], width, color);
	DrawLine(pts[FrustumPlanes::NEAR_RIGHT_BOTTOM], pts[FrustumPlanes::NEAR_RIGHT_TOP], width, color);

	DrawLine(pts[FrustumPlanes::FAR_LEFT_BOTTOM], pts[FrustumPlanes::FAR_RIGHT_BOTTOM], width, color);
	DrawLine(pts[FrustumPlanes::FAR_LEFT_TOP], pts[FrustumPlanes::FAR_RIGHT_TOP], width, color);
	DrawLine(pts[FrustumPlanes::FAR_LEFT_BOTTOM], pts[FrustumPlanes::FAR_LEFT_TOP], width, color);
	DrawLine(pts[FrustumPlanes::FAR_RIGHT_BOTTOM], pts[FrustumPlanes::FAR_RIGHT_TOP], width, color);

	DrawLine(pts[FrustumPlanes::NEAR_LEFT_BOTTOM], pts[FrustumPlanes::FAR_LEFT_BOTTOM], width, color);
	DrawLine(pts[FrustumPlanes::NEAR_RIGHT_BOTTOM], pts[FrustumPlanes::FAR_RIGHT_BOTTOM], width, color);
	DrawLine(pts[FrustumPlanes::NEAR_LEFT_TOP], pts[FrustumPlanes::FAR_LEFT_TOP], width, color);
	DrawLine(pts[FrustumPlanes::NEAR_RIGHT_TOP], pts[FrustumPlanes::FAR_RIGHT_TOP], width, color);
}

//=================================================================================================
void BasicShader::Draw()
{
	if(vertices.empty())
		return;

	// copy vertices
	{
		ReserveVertexBuffer(vertices.size());
		ResourceLock lock(vb);
		memcpy(lock.Get(), vertices.data(), sizeof(VColor) * vertices.size());
	}

	// copy indices
	{
		ReserveIndexBuffer(indices.size());
		ResourceLock lock(ib);
		memcpy(lock.Get(), indices.data(), sizeof(word) * indices.size());
	}

	// draw
	deviceContext->DrawIndexed(indices.size(), 0, 0);

	vertices.clear();
	indices.clear();
}
