#include "Pch.h"
#include "PostfxShader.h"
#include "Render.h"
#include "Engine.h"
#include "DirectX.h"

//=================================================================================================
PostfxShader::PostfxShader() : effect(nullptr), vbFullscreen(nullptr), surf(), tex()
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
}
