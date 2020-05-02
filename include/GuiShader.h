#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
class GuiShader : public ShaderHandler
{
public:
	GuiShader();
	cstring GetName() const override { return "gui"; }
	void OnInit() override;
	void OnRelease() override;
	void Prepare();
	void SetGrayscale(float value);
	void SetWrap(bool useWrap);
	void Draw(TEX tex, VGui* v, uint quads);

private:
	ID3D11DeviceContext* deviceContext;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vsGlobals;
	ID3D11Buffer* psGlobals;
	ID3D11SamplerState* samplerNormal;
	ID3D11SamplerState* samplerWrap;
	ID3D11Buffer* vb;
	TEX texEmpty;
	TEX texCurrent;
};
