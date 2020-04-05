#include "Pch.h"
#include "BasicShader.h"

#include "Camera.h"
#include "DirectX.h"
#include "Render.h"

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
	//device(app::render->GetDevice()), effect(nullptr), vb(nullptr), batch(false)
{
}



//=================================================================================================
void BasicShader::OnInit()
{
	D3D11_INPUT_ELEMENT_DESC descPos[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC descColor[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	app::render->CreateShader("basic.hlsl", descPos, countof(descPos), shaderSimple.vertexShader, shaderSimple.pixelShader, shaderSimple.layout, nullptr, "VsSimple", "PsSimple");
	app::render->CreateShader("basic.hlsl", descColor, countof(descColor), shaderColor.vertexShader, shaderColor.pixelShader, shaderColor.layout, nullptr, "VsColor", "PsColor");
	app::render->CreateShader("basic.hlsl", descPos, countof(descPos), shaderArea.vertexShader, shaderArea.pixelShader, shaderArea.layout, nullptr, "VsArea", "PsArea");

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "BasicVsGlobals");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "BasicPsGlobals");
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
	app::render->SetAlphaTest(false);
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
