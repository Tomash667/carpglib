#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class GlowShader : public ShaderHandler
{
public:
	GlowShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;

	ID3DXEffect* effect;
	D3DXHANDLE techGlowMesh, techGlowAni;
	D3DXHANDLE hMatCombined, hMatBones, hColor, hTex;
};
