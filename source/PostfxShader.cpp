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
PostfxShader::PostfxShader() : deviceContext(app::render->GetDeviceContext()), vertexShader(nullptr), pixelShader(), layout(nullptr), psGlobals(nullptr),
sampler(nullptr), targetA(nullptr), targetB(nullptr), vb(nullptr)
{
}

//=================================================================================================
void PostfxShader::OnInit()
{
	ID3DBlob* vsBlob;
	vertexShader = app::render->CreateVertexShader("postfx.hlsl", "VsEmpty", &vsBlob);
	pixelShader[POSTFX_EMPTY] = app::render->CreatePixelShader("postfx.hlsl", "PsEmpty");
	pixelShader[POSTFX_MONOCHROME] = app::render->CreatePixelShader("postfx.hlsl", "PsMonochrome");
	pixelShader[POSTFX_DREAM] = app::render->CreatePixelShader("postfx.hlsl", "PsDream");
	pixelShader[POSTFX_BLUR_X] = app::render->CreatePixelShader("postfx.hlsl", "PsBlurX");
	pixelShader[POSTFX_BLUR_Y] = app::render->CreatePixelShader("postfx.hlsl", "PsBlurY");
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
	SafeRelease(pixelShader);
	SafeRelease(layout);
	SafeRelease(psGlobals);
	SafeRelease(sampler);
	SafeRelease(targetA);
	SafeRelease(targetB);
	SafeRelease(vb);
}

void PostfxShader::Prepare()
{
	prevTarget = app::render->SetTarget(targetA);
}

void PostfxShader::Draw(const vector<PostEffect>& effects)
{
	assert(!effects.empty());

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

		deviceContext->PSSetShaderResources(0, 1, useTexA ? &targetA->tex : &targetB->tex);
		if(isLast)
			app::render->SetTarget(prevTarget);
		else
			app::render->SetTarget(useTexA ? targetB : targetA);

		deviceContext->Draw(6, 0);

		useTexA = !useTexA;
	}
}

/*: effect(nullptr), vbFullscreen(nullptr), surf(), tex()
{
}

//=================================================================================================
void PostfxShader::OnInit()
{
	effect = app::render->CompileShader("post.fx");

	techMonochrome = effect->GetTechniqueByName("Monochrome");
	techBlurX = effect->GetTechniqueByName("BlurX");
	techBlurY = effect->GetTechniqueByName("BlurY");
	techEmpty = effect->GetTechniqueByName("Empty");
	assert(techMonochrome && techBlurX && techBlurY && techEmpty);

	hTex = effect->GetParameterByName(nullptr, "t0");
	hPower = effect->GetParameterByName(nullptr, "power");
	hSkill = effect->GetParameterByName(nullptr, "skill");
	assert(hTex && hPower && hSkill);

	CreateResources();
}

//=================================================================================================
void PostfxShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
	SafeRelease(vbFullscreen);
	for(int i = 0; i < 3; ++i)
	{
		SafeRelease(surf[i]);
		SafeRelease(tex[i]);
	}
}

//=================================================================================================
void PostfxShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
	CreateResources();
}

//=================================================================================================
void PostfxShader::OnRelease()
{
	SafeRelease(effect);
	SafeRelease(vbFullscreen);
	for(int i = 0; i < 3; ++i)
	{
		SafeRelease(surf[i]);
		SafeRelease(tex[i]);
	}
}

//=================================================================================================
void PostfxShader::CreateResources()
{
	if(tex[0])
		return;

	IDirect3DDevice9* device = app::render->GetDevice();
	const Int2& wnd_size = app::engine->GetWindowSize();

	// fullscreen vertexbuffer
	VTex* v;
	V(device->CreateVertexBuffer(sizeof(VTex) * 6, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vbFullscreen, nullptr));
	V(vbFullscreen->Lock(0, sizeof(VTex) * 6, (void**)&v, 0));

	const float u_start = 0.5f / wnd_size.x;
	const float u_end = 1.f + 0.5f / wnd_size.x;
	const float v_start = 0.5f / wnd_size.y;
	const float v_end = 1.f + 0.5f / wnd_size.y;

	v[0] = VTex(-1.f, 1.f, 0.f, u_start, v_start);
	v[1] = VTex(1.f, 1.f, 0.f, u_end, v_start);
	v[2] = VTex(1.f, -1.f, 0.f, u_end, v_end);
	v[3] = VTex(1.f, -1.f, 0.f, u_end, v_end);
	v[4] = VTex(-1.f, -1.f, 0.f, u_start, v_end);
	v[5] = VTex(-1.f, 1.f, 0.f, u_start, v_start);

	V(vbFullscreen->Unlock());

	// textures/surfaces
	int ms, msq;
	app::render->GetMultisampling(ms, msq);
	D3DMULTISAMPLE_TYPE type = (D3DMULTISAMPLE_TYPE)ms;
	if(ms != D3DMULTISAMPLE_NONE)
	{
		for(int i = 0; i < 3; ++i)
		{
			V(device->CreateRenderTarget(wnd_size.x, wnd_size.y, D3DFMT_X8R8G8B8, type, msq, FALSE, &surf[i], nullptr));
			tex[i] = nullptr;
		}
		V(device->CreateTexture(wnd_size.x, wnd_size.y, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tex[0], nullptr));
	}
	else
	{
		for(int i = 0; i < 3; ++i)
		{
			V(device->CreateTexture(wnd_size.x, wnd_size.y, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tex[i], nullptr));
			surf[i] = nullptr;
		}
	}
}*/
