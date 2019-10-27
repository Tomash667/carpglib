#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class ParticleShader : public ShaderHandler
{
public:
	ParticleShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;

	ID3DXEffect* effect;
	D3DXHANDLE techParticle, techTrail;
	D3DXHANDLE hMatCombined, hTex;
};
