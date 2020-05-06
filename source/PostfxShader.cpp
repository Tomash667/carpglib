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
sampler(nullptr), vb(nullptr)
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
	pixelShaders[POSTFX_MASK] = app::render->CreatePixelShader("postfx.hlsl", "PsMask");
	layout = app::render->CreateInputLayout(VDI_TEX, vsBlob, "PostfxLayout");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "PostfxPsGlobals");
	sampler = app::render->CreateSampler(Render::TEX_ADR_CLAMP);
	vsBlob->Release();

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
	SafeRelease(vb);
}

//=================================================================================================
void PostfxShader::SetTarget()
{
	TEX texEmpty[] = { nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 2, texEmpty);

	prevTarget = app::render->GetRenderTarget();
	activeTarget = GetTarget(true);
	ID3D11RenderTargetView* renderTargetView = activeTarget->GetRenderTargetView();
	deviceContext->OMSetRenderTargets(1, &renderTargetView, app::render->GetDepthStencilView());
	app::render->SetViewport(activeTarget->GetSize());
}

//=================================================================================================
RenderTarget* PostfxShader::GetTarget(bool ms)
{
	if(ms && app::render->IsMultisamplingEnabled())
	{
		if(targetsMS.empty())
			return app::render->CreateRenderTarget(Int2::Zero, RenderTarget::F_NO_DEPTH | RenderTarget::F_NO_DRAW);
		RenderTarget* target = targetsMS.back();
		targetsMS.pop_back();
		return target;
	}
	else
	{
		if(targets.empty())
			return app::render->CreateRenderTarget(Int2::Zero, RenderTarget::F_NO_DEPTH | RenderTarget::F_NO_MS);
		RenderTarget* target = targets.back();
		targets.pop_back();
		return target;
	}
}

//=================================================================================================
void PostfxShader::FreeTarget(RenderTarget* target)
{
	if(IsSet(target->GetFlags(), RenderTarget::F_NO_DRAW))
		targetsMS.push_back(target);
	else
		targets.push_back(target);
}

//=================================================================================================
RenderTarget* PostfxShader::RequestActiveTarget()
{
	assert(activeTarget);
	RenderTarget* target = activeTarget;
	activeTarget = nullptr;
	return target;
}

//=================================================================================================
RenderTarget* PostfxShader::ResolveTarget(RenderTarget* target)
{
	if(IsSet(target->GetFlags(), RenderTarget::F_NO_DRAW))
	{
		RenderTarget* targetNormal = GetTarget(false);
		ID3D11Texture2D* texMS, *texNormal;
		target->GetRenderTargetView()->GetResource(reinterpret_cast<ID3D11Resource**>(&texMS));
		targetNormal->tex->GetResource(reinterpret_cast<ID3D11Resource**>(&texNormal));
		deviceContext->ResolveSubresource(texNormal, 0, texMS, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		texMS->Release();
		texNormal->Release();
		if(activeTarget == target)
			activeTarget = targetNormal;
		FreeTarget(target);
		return targetNormal;
	}
	else
		return target;
}

//=================================================================================================
void PostfxShader::Prepare()
{
	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_NO);
	app::render->SetRasterState(Render::RASTER_NORMAL);

	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	deviceContext->PSSetSamplers(0, 1, &sampler);
	uint stride = sizeof(VTex), offset = 0;
	deviceContext->IASetInputLayout(layout);
	deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
}

//=================================================================================================
RenderTarget* PostfxShader::Draw(const vector<PostEffect>& effects, RenderTarget* input, bool finalStage)
{
	assert(!effects.empty());

	// resolve MS if required
	if(!input)
		input = activeTarget;
	input = ResolveTarget(input);
	assert(input);

	RenderTarget* currentInput = input;
	RenderTarget* tmpTarget = nullptr;
	RenderTarget* tmpSource = nullptr;

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

		TEX tex[] = { nullptr, nullptr };
		deviceContext->PSSetShaderResources(0, 2, tex);
		if(isLast && finalStage)
		{
			if(tmpTarget)
			{
				FreeTarget(tmpTarget);
				tmpTarget = nullptr;
			}
			app::render->SetRenderTarget(prevTarget);
		}
		else
		{
			tmpTarget = GetTarget(false);
			ID3D11RenderTargetView* renderTargetView = tmpTarget->GetRenderTargetView();
			deviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);
		}
		tex[0] = currentInput->tex;
		tex[1] = effect.tex;
		deviceContext->PSSetShader(pixelShaders[effect.id], nullptr, 0);
		deviceContext->PSSetShaderResources(0, 2, tex);

		deviceContext->Draw(6, 0);

		if(tmpTarget)
		{
			if(tmpSource)
				FreeTarget(tmpSource);
			tmpSource = tmpTarget;
			tmpTarget = nullptr;
			currentInput = tmpSource;
		}
	}

	if(tmpSource && finalStage)
	{
		FreeTarget(tmpSource);
		tmpSource = nullptr;
	}
	if(activeTarget)
		FreeTarget(activeTarget);
	return tmpSource;
}

//=================================================================================================
void PostfxShader::Merge(RenderTarget* inputA, RenderTarget* inputB, RenderTarget* output)
{
	assert(inputA && inputB);

	ID3D11RenderTargetView* renderTargetView;
	RenderTarget* tmpTarget = nullptr;
	if(!output)
	{
		if(prevTarget)
		{
			tmpTarget = GetTarget(false);
			renderTargetView = tmpTarget->GetRenderTargetView();
		}
		else
			renderTargetView = app::render->GetRenderTargetView();
	}
	else
		renderTargetView = output->GetRenderTargetView();

	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	deviceContext->PSSetSamplers(0, 1, &sampler);
	uint stride = sizeof(VTex), offset = 0;
	deviceContext->IASetInputLayout(layout);
	deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	TEX tex[] = { nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 2, tex);
	deviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);
	deviceContext->PSSetShader(pixelShaders[POSTFX_EMPTY], nullptr, 0);

	tex[0] = inputA->tex;
	deviceContext->PSSetShaderResources(0, 2, tex);
	deviceContext->Draw(6, 0);

	app::render->SetBlendState(Render::BLEND_ADD_ONE2);
	tex[0] = inputB->tex;
	deviceContext->PSSetShaderResources(0, 2, tex);
	deviceContext->Draw(6, 0);
	app::render->SetBlendState(Render::BLEND_NO);

	if(tmpTarget)
	{
		renderTargetView = prevTarget->GetRenderTargetView();
		deviceContext->OMSetRenderTargets(1, &renderTargetView, prevTarget->GetDepthStencilView());
		tex[0] = tmpTarget->tex;
		deviceContext->PSSetShaderResources(0, 2, tex);
		app::render->SetViewport(prevTarget->GetSize());

		deviceContext->Draw(6, 0);
		FreeTarget(tmpTarget);
	}
	else if(output)
		activeTarget = output;
	else
		app::render->SetRenderTarget(nullptr);
}
