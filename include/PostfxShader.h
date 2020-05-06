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
	POSTFX_MASK,
	POSTFX_MAX
};

//-----------------------------------------------------------------------------
struct PostEffect
{
	PostEffect() : tex(nullptr) {}

	PostEffectId id;
	float power;
	Vec4 skill;
	TEX tex;
};

//-----------------------------------------------------------------------------
class PostfxShader : public ShaderHandler
{
public:
	PostfxShader();
	cstring GetName() const override { return "postfx"; }
	void OnInit() override;
	void OnRelease() override;
	void SetTarget();
	void Prepare();
	RenderTarget* Draw(const vector<PostEffect>& effects, RenderTarget* input = nullptr, bool finalStage = true);
	void Merge(RenderTarget* inputA, RenderTarget* inputB, RenderTarget* output);
	RenderTarget* GetTarget(bool ms);
	void FreeTarget(RenderTarget* target);
	RenderTarget* RequestActiveTarget();
	RenderTarget* ResolveTarget(RenderTarget* target);

private:
	ID3D11DeviceContext* deviceContext;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShaders[POSTFX_MAX];
	ID3D11InputLayout* layout;
	ID3D11Buffer* psGlobals;
	ID3D11SamplerState* sampler;
	ID3D11Buffer* vb;
	RenderTarget* prevTarget;
	RenderTarget* activeTarget;
	vector<RenderTarget*> targets;
	vector<RenderTarget*> targetsMS;
};
