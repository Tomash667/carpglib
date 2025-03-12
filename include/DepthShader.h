#pragma once

//-----------------------------------------------------------------------------
#include "MeshShape.h"
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
class DepthShader final : public ShaderHandler
{
public:
	DepthShader();
	cstring GetName() const override { return "depth"; }
	void OnInit() override;
	void OnRelease() override;
	void Prepare(const Camera& camera);
	void Draw(SceneNode* node);
	void DrawCustom(ID3D11Buffer* vb, ID3D11Buffer* ib, uint startIndex, uint indexCount);

private:
	ID3D11DeviceContext* deviceContext;
	Ptr<ID3D11VertexShader> vertexShaderMesh, vertexShaderAni;
	ID3D11PixelShader* pixelShader;
	Ptr<ID3D11InputLayout> layoutMesh, layoutMeshTangent, layoutMeshWeight, layoutMeshTangentWeight, layoutAni, layoutAniTangent;
	ID3D11Buffer* vsLocals;
	Matrix matViewProj;
};
