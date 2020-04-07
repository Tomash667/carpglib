#include "Pch.h"
#include "GuiShader.h"

#include "DirectX.h"
#include "Engine.h"
#include "Render.h"
#include "Texture.h"

struct VsGlobals
{
	Vec2 size;
};

struct PsGlobals
{
	float grayscale;
};

//=================================================================================================
GuiShader::GuiShader() : deviceContext(app::render->GetDeviceContext()), vertexShader(nullptr), pixelShader(nullptr),
layout(nullptr), vsGlobals(nullptr), psGlobals(nullptr), sampler(nullptr), vb(nullptr), texEmpty(nullptr), texCurrent(nullptr)
{
}

//=================================================================================================
void GuiShader::OnInit()
{
	app::render->CreateShader("gui.hlsl", VDI_PARTICLE, vertexShader, pixelShader, layout);

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "GuiVsGlobals");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "GuiPsGlobals");
	sampler = app::render->CreateSampler(Render::TEX_ADR_CLAMP, true);

	texEmpty = app::render->CreateRawTexture(Int2(1, 1), &Color::White);

	// create vertex buffer
	D3D11_BUFFER_DESC bufDesc;
	bufDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufDesc.ByteWidth = sizeof(VParticle) * 6 * 256;
	bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufDesc.MiscFlags = 0;
	bufDesc.StructureByteStride = 0;

	V(app::render->GetDevice()->CreateBuffer(&bufDesc, nullptr, &vb));
#ifdef _DEBUG
	vb->SetPrivateData(WKPDID_D3DDebugObjectName, strlen("GuiVb"), "GuiVb");
#endif
}

//=================================================================================================
void GuiShader::OnRelease()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShader);
	SafeRelease(layout);
	SafeRelease(vsGlobals);
	SafeRelease(psGlobals);
	SafeRelease(sampler);
	SafeRelease(vb);
	SafeRelease(texEmpty);
}

//=================================================================================================
void GuiShader::Prepare()
{
	app::render->SetAlphaTest(false);
	app::render->SetAlphaBlend(true);
	app::render->SetDepthState(Render::DEPTH_NO);
	app::render->SetNoCulling(true);

	// setup shader
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	deviceContext->PSSetSamplers(0, 1, &sampler);
	deviceContext->PSSetShaderResources(0, 1, &texEmpty);
	uint stride = sizeof(VParticle),
		offset = 0;
	deviceContext->IASetInputLayout(layout);
	deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

	// vertex shader constants
	ResourceLock lock(vsGlobals, D3D11_MAP_WRITE_DISCARD);
	lock.Get<VsGlobals>()->size = Vec2(app::engine->GetWindowSize());

	SetGrayscale(0.f);
	texCurrent = nullptr;
}

//=================================================================================================
void GuiShader::SetGrayscale(float value)
{
	assert(InRange(value, 0.f, 1.f));
	ResourceLock lock(psGlobals, D3D11_MAP_WRITE_DISCARD);
	lock.Get<PsGlobals>()->grayscale = value;
}

//=================================================================================================
void GuiShader::Draw(TEX tex, VParticle* v, uint quads)
{
	assert(v && quads >= 1 && quads <= 256);

	// copy vertices
	{
		ResourceLock lock(vb, D3D11_MAP_WRITE_DISCARD);
		memcpy(lock.Get(), v, sizeof(VParticle) * 6 * quads);
	}

	if(!tex)
		tex = texEmpty;
	if(tex != texCurrent)
	{
		texCurrent = tex;
		deviceContext->PSSetShaderResources(0, 1, &tex);
	}

	deviceContext->Draw(quads * 6, 0);
}
