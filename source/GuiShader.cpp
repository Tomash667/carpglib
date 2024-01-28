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
GuiShader::GuiShader() : deviceContext(app::render->GetDeviceContext()), vertexShader(nullptr), pixelShader(nullptr), layout(nullptr), vsGlobals(nullptr),
psGlobals(nullptr), samplerNormal(nullptr), samplerWrap(nullptr), vb(nullptr), texEmpty(nullptr), texCurrent(nullptr)
{
}

//=================================================================================================
void GuiShader::OnInit()
{
	Render::ShaderParams params;
	params.name = "gui";
	params.decl = VDI_GUI;
	params.vertexShader = &vertexShader;
	params.pixelShader = &pixelShader;
	params.layout = &layout;
	app::render->CreateShader(params);

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "GuiVsGlobals");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "GuiPsGlobals");
	samplerNormal = app::render->CreateSampler(Render::TEX_ADR_CLAMP, true);
	samplerWrap = app::render->CreateSampler(Render::TEX_ADR_WRAP, true);

	texEmpty = app::render->CreateImmutableTexture(Int2(1, 1), &Color::White);

	// create vertex buffer
	D3D11_BUFFER_DESC bufDesc;
	bufDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufDesc.ByteWidth = sizeof(VGui) * 6 * 256;
	bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufDesc.MiscFlags = 0;
	bufDesc.StructureByteStride = 0;

	V(app::render->GetDevice()->CreateBuffer(&bufDesc, nullptr, &vb));
	SetDebugName(vb, "GuiVb");
}

//=================================================================================================
void GuiShader::OnRelease()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShader);
	SafeRelease(layout);
	SafeRelease(vsGlobals);
	SafeRelease(psGlobals);
	SafeRelease(samplerNormal);
	SafeRelease(samplerWrap);
	SafeRelease(vb);
	SafeRelease(texEmpty);
}

//=================================================================================================
void GuiShader::Prepare(const Int2& size)
{
	app::render->SetBlendState(Render::BLEND_ADD);
	app::render->SetDepthState(Render::DEPTH_NO);
	app::render->SetRasterState(Render::RASTER_NO_CULLING);

	// setup shader
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	deviceContext->PSSetSamplers(0, 1, &samplerNormal);
	deviceContext->PSSetShaderResources(0, 1, &texEmpty);
	uint stride = sizeof(VGui),
		offset = 0;
	deviceContext->IASetInputLayout(layout);
	deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

	// vertex shader constants
	ResourceLock lock(vsGlobals);
	lock.Get<VsGlobals>()->size = Vec2(size);

	SetGrayscale(0.f);
	texCurrent = nullptr;
}

//=================================================================================================
void GuiShader::SetGrayscale(float value)
{
	assert(InRange(value, 0.f, 1.f));
	ResourceLock lock(psGlobals);
	lock.Get<PsGlobals>()->grayscale = value;
}

//=================================================================================================
void GuiShader::SetWrap(bool useWrap)
{
	ID3D11SamplerState* sampler = useWrap ? samplerWrap : samplerNormal;
	deviceContext->PSSetSamplers(0, 1, &sampler);
}

//=================================================================================================
void GuiShader::Draw(TEX tex, VGui* v, uint quads)
{
	assert(v && quads >= 1 && quads <= 256);

	// copy vertices
	{
		ResourceLock lock(vb);
		memcpy(lock.Get(), v, sizeof(VGui) * 6 * quads);
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
