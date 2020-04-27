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
	PostEffectId id;
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
	void Prepare(bool dual);
	int Draw(const vector<PostEffect>& effects, bool finalStage, bool useTexB);
	void Merge(int targetA, int targetB, int output);
	RenderTarget* GetTarget(uint index) { return targets[index]; }

private:
	int GetFreeTarget(int targetA, int targetB);

	ID3D11DeviceContext* deviceContext;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShaders[POSTFX_MAX];
	ID3D11InputLayout* layout;
	ID3D11Buffer* psGlobals;
	ID3D11SamplerState* sampler;
	RenderTarget* targets[3];
	ID3D11Buffer* vb;
	RenderTarget* prevTarget;
};
