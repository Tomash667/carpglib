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
sampler(nullptr), targets(), vb(nullptr)
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

	// create render targets
	const Int2& wndSize = app::engine->GetWindowSize();
	for(int i = 0; i < 3; ++i)
		targets[i] = app::render->CreateRenderTarget(wndSize, false);

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
	SafeRelease(targets);
	SafeRelease(vb);
}

//=================================================================================================
void PostfxShader::Prepare(bool dual)
{
	prevTarget = app::render->GetRenderTarget();
	ID3D11RenderTargetView* renderTargetView = targets[dual ? 2 : 0]->GetRenderTargetView();
	deviceContext->OMSetRenderTargets(1, &renderTargetView, app::render->GetDepthStencilView());
	app::render->SetViewport(targets[0]->GetSize());
}

//=================================================================================================
int PostfxShader::Draw(const vector<PostEffect>& effects, bool finalStage, bool useTexB)
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

	bool useTexA = !useTexB;

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
		if(isLast && finalStage)
			app::render->SetRenderTarget(prevTarget);
		else
		{
			ID3D11RenderTargetView* renderTargetView = targets[useTexA ? 1 : 0]->GetRenderTargetView();
			deviceContext->OMSetRenderTargets(1, &renderTargetView, app::render->GetDepthStencilView());
		}
		deviceContext->PSSetShader(pixelShaders[effect.id], nullptr, 0);
		deviceContext->PSSetShaderResources(0, 1, &targets[useTexA ? 0 : 1]->tex);

		deviceContext->Draw(6, 0);

		useTexA = !useTexA;
	}

	if(finalStage)
		return -1;
	else
		return useTexA ? 0 : 1;
}

//=================================================================================================
void PostfxShader::Merge(int targetA, int targetB, int output)
{
	assert(InRange(targetA, 0, 2));
	assert(InRange(targetB, 0, 2));
	assert(InRange(output, -1, 2));
	assert(targetA != targetB && targetA != output);

	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_NO);
	app::render->SetRasterState(Render::RASTER_NORMAL);

	ID3D11RenderTargetView* renderTargetView;
	int tmpOutput;
	if(output == -1)
	{
		if(prevTarget)
		{
			tmpOutput = GetFreeTarget(targetA, targetB);
			renderTargetView = targets[tmpOutput]->GetRenderTargetView();
		}
		else
			renderTargetView = app::render->GetRenderTargetView();
	}
	else
		renderTargetView = targets[output]->GetRenderTargetView();

	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	deviceContext->PSSetSamplers(0, 1, &sampler);
	uint stride = sizeof(VTex), offset = 0;
	deviceContext->IASetInputLayout(layout);
	deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	TEX texEmpty = nullptr;
	deviceContext->PSSetShaderResources(0, 1, &texEmpty);
	deviceContext->OMSetRenderTargets(1, &renderTargetView, app::render->GetDepthStencilView());
	deviceContext->PSSetShader(pixelShaders[POSTFX_EMPTY], nullptr, 0);

	deviceContext->PSSetShaderResources(0, 1, &targets[targetA]->tex);
	deviceContext->Draw(6, 0);

	app::render->SetBlendState(Render::BLEND_ADD_ONE2);
	app::render->SetDepthState(Render::DEPTH_STENCIL_KEEP);
	deviceContext->PSSetShaderResources(0, 1, &targets[targetB]->tex);
	deviceContext->Draw(6, 0);

	if(prevTarget && output == -1)
	{
		renderTargetView = prevTarget->GetRenderTargetView();
		deviceContext->OMSetRenderTargets(1, &renderTargetView, prevTarget->GetDepthStencilView());
		deviceContext->PSSetShaderResources(0, 1, &targets[tmpOutput]->tex);
		app::render->SetViewport(prevTarget->GetSize());

		app::render->SetBlendState(Render::BLEND_NO);
		app::render->SetDepthState(Render::DEPTH_NO);

		deviceContext->Draw(6, 0);
	}
}

//=================================================================================================
int PostfxShader::GetFreeTarget(int targetA, int targetB)
{
	switch(targetA + targetB)
	{
	case 1: // 0,1 used
		return 2;
	case 2: // 0,2 used
		return 1;
	case 3: // 1,2 used
		return 0;
	default:
		assert(0);
		return 0;
	}
}
