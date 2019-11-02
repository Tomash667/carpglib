#include "EnginePch.h"
#include "EngineCore.h"
#include "PostfxShader.h"
#include "Render.h"
#include "Engine.h"
#include "DirectX.h"

//=================================================================================================
PostfxShader::PostfxShader() : effect(nullptr), surf(), tex()
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

	CreateTextures();
}

//=================================================================================================
void PostfxShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
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
	CreateTextures();
}

//=================================================================================================
void PostfxShader::OnRelease()
{
	SafeRelease(effect);
	for(int i = 0; i < 3; ++i)
	{
		SafeRelease(surf[i]);
		SafeRelease(tex[i]);
	}
}

//=================================================================================================
void PostfxShader::CreateTextures()
{
	if(tex[0])
		return;

	IDirect3DDevice9* device = app::render->GetDevice();
	const Int2& wnd_size = app::engine->GetWindowSize();
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
