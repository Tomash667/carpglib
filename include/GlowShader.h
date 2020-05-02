#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class GlowShader : public ShaderHandler
{
public:
	GlowShader(PostfxShader* postfx);
	cstring GetName() const override { return "glow"; }
	void OnInit() override;
	void OnRelease() override;
	void Draw(Camera& camera, const vector<GlowNode>& glowNodes, bool usePostfx);

private:
	void DrawGlowNodes(Camera& camera, const vector<GlowNode>& glowNodes);

	PostfxShader* postfxShader;
	ID3D11DeviceContext* deviceContext;
	ID3D11VertexShader* vertexShaderMesh;
	ID3D11VertexShader* vertexShaderAni;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layoutMesh;
	ID3D11InputLayout* layoutMeshTangent;
	ID3D11InputLayout* layoutMeshWeight;
	ID3D11InputLayout* layoutMeshTangentWeight;
	ID3D11InputLayout* layoutAni;
	ID3D11InputLayout* layoutAniTangent;
	ID3D11Buffer* vsGlobals;
	ID3D11Buffer* psGlobals;
};
