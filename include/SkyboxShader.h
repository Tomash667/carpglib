#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class SkyboxShader : public ShaderHandler
{
public:
	SkyboxShader();
	cstring GetName() const override { return "skybox"; }
	void OnInit() override;
	void OnRelease() override;
	void Draw(Mesh& mesh, Camera& camera);

private:
	ID3D11DeviceContext* deviceContext;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vsGlobals;
	ID3D11SamplerState* sampler;
};
