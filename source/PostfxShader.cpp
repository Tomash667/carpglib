#include "Pch.h"
#include "PostfxShader.h"

#include "DirectX.h"
#include "Engine.h"
#include "Render.h"
#include "RenderTarget.h"
#include "VertexDeclaration.h"

struct PsGlobals
{
	Vec4 skill;
	float power;
	float time;
};

//=================================================================================================
PostfxShader::PostfxShader() : deviceContext(app::render->GetDeviceContext()), vertexShader(nullptr), pixelShaders(), layout(nullptr), psGlobals(nullptr),
sampler(nullptr), targetA(nullptr), targetB(nullptr), vb(nullptr)
{
}

//=================================================================================================
void PostfxShader::OnInit()
{
	ID3DBlob* vsBlob;
	vertexShader = app::render->CreateVertexShader("postfx.hlsl", "VsEmpty", &vsBlob);
	pixelShaders[POSTFX_EMPTY] = app::render->CreatePixelShader("postfx.hlsl", "PsEmpty");
	pixelShaders[POSTFX_MONOCHROME] = app::render->CreatePixelShader("postfx.hlsl", "PsMonochrome");
	pixelShaders[POSTFX_DREAM] = app::render->CreatePixelShader("postfx.hlsl", "PsDream");
	pixelShaders[POSTFX_BLUR_X] = app::render->CreatePixelShader("postfx.hlsl", "PsBlurX");
	pixelShaders[POSTFX_BLUR_Y] = app::render->CreatePixelShader("postfx.hlsl", "PsBlurY");
	layout = app::render->CreateInputLayout(VDI_TEX, vsBlob, "PostfxLayout");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "PostfxPsGlobals");
	sampler = app::render->CreateSampler(Render::TEX_ADR_CLAMP);
	vsBlob->Release();

	const Int2& wndSize = app::engine->GetWindowSize();
	targetA = app::render->CreateRenderTarget(wndSize);
	targetB = app::render->CreateRenderTarget(wndSize);

	// create fullscreen vertex buffer
	const VTex v[] = {
		VTex(-1.f, 1.f, 0.f, 0, 0),
		VTex(1.f, 1.f, 0.f, 1, 0),
		VTex(1.f, -1.f, 0.f, 1, 1),
		VTex(1.f, -1.f, 0.f, 1, 1),
		VTex(-1.f, -1.f, 0.f, 0, 1),
		VTex(-1.f, 1.f, 0.f, 0, 0)
	};
	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = sizeof(VTex) * 6;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = v;

	V(app::render->GetDevice()->CreateBuffer(&desc, &data, &vb));
}

//=================================================================================================
void PostfxShader::OnRelease()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShaders);
	SafeRelease(layout);
	SafeRelease(psGlobals);
	SafeRelease(sampler);
	SafeRelease(targetA);
	SafeRelease(targetB);
	SafeRelease(vb);
}

//=================================================================================================
void PostfxShader::Prepare()
{
	prevTarget = app::render->SetTarget(targetA);
}

//=================================================================================================
void PostfxShader::Draw(const vector<PostEffect>& effects)
{
	assert(!effects.empty());

	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_NO);
	app::render->SetRasterState(Render::RASTER_NORMAL);

	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	deviceContext->PSSetSamplers(0, 1, &sampler);
	uint stride = sizeof(VTex), offset = 0;
	deviceContext->IASetInputLayout(layout);
	deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

	bool useTexA = true;

	for(uint i = 0, count = effects.size(); i < count; ++i)
	{
		const PostEffect& effect = effects[i];
		const bool isLast = (i + 1 == count);

		// set pixel shader params
		{
			ResourceLock lock(psGlobals);
			PsGlobals& psg = *lock.Get<PsGlobals>();
			psg.power = effect.power;
			psg.skill = effect.skill;
		}

		TEX texEmpty = nullptr;
		deviceContext->PSSetShaderResources(0, 1, &texEmpty);
		if(isLast)
			app::render->SetTarget(prevTarget);
		else
			app::render->SetTarget(useTexA ? targetB : targetA);
		deviceContext->PSSetShader(pixelShaders[effect.id], nullptr, 0);
		deviceContext->PSSetShaderResources(0, 1, useTexA ? &targetA->tex : &targetB->tex);

		deviceContext->Draw(6, 0);

		useTexA = !useTexA;
	}
}
