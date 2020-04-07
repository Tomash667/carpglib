#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class GlowShader : public ShaderHandler
{
public:
	GlowShader();
	cstring GetName() const override { return "glow"; }
	void OnInit() override;
	void OnRelease() override;
	void Begin();
	void End();

private:
	ID3D11VertexShader* vertexShaderMesh;
	ID3D11VertexShader* vertexShaderAni;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layoutMesh;
	ID3D11InputLayout* layoutMeshTangent;
	ID3D11InputLayout* layoutAni;
	ID3D11InputLayout* layoutAniTangent;
	ID3D11Buffer* vsGlobals;
	ID3D11Buffer* psGlobals;
	ID3D11SamplerState* sampler;
};
