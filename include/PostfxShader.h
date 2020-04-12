#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
enum PostEffectId
{
	POSTFX_EMPTY,
	POSTFX_MONOCHROME,
	POSTFX_DREAM,
	POSTFX_BLUR_X,
	POSTFX_BLUR_Y,
	POSTFX_MAX
};

//-----------------------------------------------------------------------------
struct PostEffect
{
	int id;
	//D3DXHANDLE tech;
	float power;
	Vec4 skill;
};

//-----------------------------------------------------------------------------
class PostfxShader : public ShaderHandler
{
public:
	PostfxShader();
	cstring GetName() const override { return "postfx"; }
	void OnInit() override;
	void OnRelease() override;

	/*ID3DXEffect* effect;
	D3DXHANDLE techMonochrome, techBlurX, techBlurY, techEmpty;
	D3DXHANDLE hTex, hPower, hSkill;
	VB vbFullscreen;
	// post effect uses 3 textures or if multisampling is enabled 3 surfaces and 1 texture
	SURFACE surf[3];*/
	TEX tex[3];

private:
	void CreateResources();

	ID3D11DeviceContext* deviceContext;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader[POSTFX_MAX];
	ID3D11InputLayout* layout;
	ID3D11SamplerState* sampler;
};
