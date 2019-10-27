#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class PostfxShader : public ShaderHandler
{
public:
	PostfxShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;

	ID3DXEffect* effect;
	D3DXHANDLE techMonochrome, techBlurX, techBlurY, techEmpty;
	D3DXHANDLE hTex, hPower, hSkill;
};
