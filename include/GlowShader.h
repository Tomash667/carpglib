#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class GlowShader : public ShaderHandler
{
public:
	GlowShader();
	cstring GetName() const override { return "glow"; }
	void OnInit() override;
	void OnRelease() override;

	/*ID3DXEffect* effect;
	D3DXHANDLE techGlowMesh, techGlowAni;
	D3DXHANDLE hMatCombined, hMatBones, hColor, hTex;*/
};
