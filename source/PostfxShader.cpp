#include "EnginePch.h"
#include "EngineCore.h"
#include "PostfxShader.h"
#include "Render.h"
#include "DirectX.h"

//=================================================================================================
PostfxShader::PostfxShader() : effect(nullptr)
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
}

//=================================================================================================
void PostfxShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
}

//=================================================================================================
void PostfxShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void PostfxShader::OnRelease()
{
	SafeRelease(effect);
}
