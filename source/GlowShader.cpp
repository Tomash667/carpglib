#include "Pch.h"
#include "GlowShader.h"

#include "DirectX.h"
#include "Mesh.h"
#include "Render.h"

struct VsLocals
{
	Matrix matCombined;
	Matrix matBones[Mesh::MAX_BONES];
};

struct PsLocals
{
	Vec4 color;
};

//=================================================================================================
GlowShader::GlowShader() : vertexShaderMesh(nullptr), vertexShaderAni(nullptr), pixelShader(nullptr), layoutMesh(nullptr), layoutAni(nullptr),
sampler(nullptr), vsLocals(nullptr), psLocals(nullptr)
{
}

void GlowShader::OnInit()
{
	app::render->CreateShader("glow.hlsl", )
}

void GlowShader::OnRelease()
{
	SafeRelease(vertexShaderMesh);
	SafeRelease(vertexShaderAni);
	SafeRelease(pixelShader);
	SafeRelease(layoutMesh);
	SafeRelease(layoutAni);
	SafeRelease(sampler);
	SafeRelease(vsLocals);
	SafeRelease(psLocals);
}

FIXME;
/*
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
