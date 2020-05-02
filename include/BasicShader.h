#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
class BasicShader : public ShaderHandler
{
	struct Shader
	{
		ID3D11VertexShader* vertexShader;
		ID3D11PixelShader* pixelShader;
		ID3D11InputLayout* layout;

		Shader() : vertexShader(nullptr), pixelShader(nullptr), layout(nullptr) {}
		void Release();
	};
public:
	BasicShader();
	cstring GetName() const override { return "basic"; }
	void OnInit() override;
	void OnRelease() override;

	// drawing mesh debug nodes
	void DrawDebugNodes(const vector<DebugNode*>& nodes);

	// drawing vertices
	void Prepare(const Camera& camera);
	void SetAreaParams(const Vec3& playerPos, float range, float falloff);
	void DrawQuad(const Vec3(&pts)[4], Color color);
	void DrawArea(const vector<Vec3>& vertices, const vector<word>& indices, Color color);
	void Draw();

private:
	void ReserveVertexBuffer(uint vertexCount);
	void ReserveIndexBuffer(uint indexCount);

	ID3D11DeviceContext* deviceContext;
	Shader shaderMesh, shaderColor;
	ID3D11Buffer* vsGlobals;
	ID3D11Buffer* psGlobalsMesh;
	ID3D11Buffer* psGlobalsColor;
	MeshPtr meshes[4];
	ID3D11Buffer* vb;
	ID3D11Buffer* ib;
	uint vbSize, ibSize;
	vector<VColor> vertices;
	vector<word> indices;
};
