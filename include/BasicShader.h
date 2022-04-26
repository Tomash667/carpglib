#pragma once

//-----------------------------------------------------------------------------
#include "MeshShape.h"
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
	void PrepareForShapes(const Camera& camera);
	void DrawDebugNodes(const vector<DebugNode*>& nodes);
	void DrawShape(MeshShape shape, const Matrix& m, Color color);

	// drawing vertices
	void Prepare(const Camera& camera);
	void SetAreaParams(const Vec3& playerPos, float range, float falloff);
	void DrawQuad(const array<Vec3, 4>& pts, Color color);
	void DrawArea(const vector<Vec3>& vertices, const vector<word>& indices, Color color);
	void DrawLine(const Vec3& from, const Vec3& to, float width, Color color);
	void DrawFrustum(const FrustumPlanes& frustum, float width, Color color);
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
	Vec3 camPos;
	Color prevColor;
	MeshShape prevShape;
	Matrix matViewProj;
};
