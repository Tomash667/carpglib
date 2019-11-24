#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
struct PostEffect
{
	int id;
	D3DXHANDLE tech;
	float power;
	Vec4 skill;
};

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
	VB vbFullscreen;
	// post effect uses 3 textures or if multisampling is enabled 3 surfaces and 1 texture
	SURFACE surf[3];
	TEX tex[3];

private:
	void CreateResources();
};
