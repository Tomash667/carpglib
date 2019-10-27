#include "EnginePch.h"
#include "EngineCore.h"
#include "ParticleShader.h"
#include "Render.h"
#include "DirectX.h"

//=================================================================================================
ParticleShader::ParticleShader() : effect(nullptr)
{
}

//=================================================================================================
void ParticleShader::OnInit()
{
	effect = app::render->CompileShader("particle.fx");

	techParticle = effect->GetTechniqueByName("particle");
	techTrail = effect->GetTechniqueByName("trail");
	assert(techParticle && techTrail);

	hMatCombined = effect->GetParameterByName(nullptr, "matCombined");
	hTex = effect->GetParameterByName(nullptr, "tex0");
	assert(hMatCombined && hTex);
}

//=================================================================================================
void ParticleShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
}

//=================================================================================================
void ParticleShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void ParticleShader::OnRelease()
{
	SafeRelease(effect);
}
