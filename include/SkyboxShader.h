#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class SkyboxShader : public ShaderHandler
{
public:
	SkyboxShader();
	void OnInit() override;
	void OnRelease() override;
	cstring GetName() const override { return "skybox"; }
	void Draw(Mesh& mesh, Camera& camera);

private:
	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vs_globals;
	ID3D11SamplerState* sampler;
};
