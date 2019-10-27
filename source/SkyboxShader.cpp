#include "EnginePch.h"
#include "EngineCore.h"
#include "SkyboxShader.h"
#include "Render.h"
#include "DirectX.h"

//=================================================================================================
SkyboxShader::SkyboxShader() : effect(nullptr)
{
}

//=================================================================================================
void SkyboxShader::OnInit()
{
	effect = app::render->CompileShader("skybox.fx");

	tech = effect->GetTechniqueByName("skybox");
	assert(tech);

	hMatCombined = effect->GetParameterByName(nullptr, "matCombined");
	hTex = effect->GetParameterByName(nullptr, "tex0");
	assert(hMatCombined && hTex);
}

//=================================================================================================
void SkyboxShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
}

//=================================================================================================
void SkyboxShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void SkyboxShader::OnRelease()
{
	SafeRelease(effect);
}
