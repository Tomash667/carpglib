#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class GuiShader : public ShaderHandler
{
public:
	GuiShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;

	ID3DXEffect* effect;
	D3DXHANDLE techTex, techColor, techGrayscale;
	D3DXHANDLE hSize, hTex;
	VB vb, vb2;

private:
	void CreateVertexBuffer();
};
