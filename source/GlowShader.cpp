#include "Pch.h"
/*#include "GlowShader.h"
#include "Render.h"
#include "DirectX.h"

//=================================================================================================
GlowShader::GlowShader() : effect(nullptr)
{
}

//=================================================================================================
void GlowShader::OnInit()
{
	effect = app::render->CompileShader("glow.fx");

	techGlowMesh = effect->GetTechniqueByName("mesh");
	techGlowAni = effect->GetTechniqueByName("ani");
	assert(techGlowMesh && techGlowAni);

	hMatCombined = effect->GetParameterByName(nullptr, "matCombined");
	hMatBones = effect->GetParameterByName(nullptr, "matBones");
	hColor = effect->GetParameterByName(nullptr, "color");
	hTex = effect->GetParameterByName(nullptr, "texDiffuse");
	assert(hMatCombined && hMatBones && hColor && hTex);
}

//=================================================================================================
void GlowShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
}

//=================================================================================================
void GlowShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void GlowShader::OnRelease()
{
	SafeRelease(effect);
}*/
