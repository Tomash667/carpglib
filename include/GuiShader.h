#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class GuiShader : public ShaderHandler
{
public:
	GuiShader();
	void OnInit() override;
	void OnRelease() override;
	cstring GetName() const override { return "gui"; }

	/*ID3DXEffect* effect;
	D3DXHANDLE techTex, techColor, techGrayscale;
	D3DXHANDLE hSize, hTex;
	VB vb, vb2;*/

private:
	void CreateVertexBuffer();

	ID3D11SamplerState* sampler;
};
