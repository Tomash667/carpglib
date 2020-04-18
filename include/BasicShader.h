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

	// drawing debug nodes
	void DrawDebugNodes(const vector<DebugNode*>& nodes);

	// drawing areas
	void PrepareArea(const Camera& camera, const Vec3& playerPos);
	void SetAreaParams(Color color, float range);
	void DrawArea(const Vec3(&pts)[4]);
	void DrawArea(const vector<Vec3>& vertices, const vector<word>& indices);
	/*void Prepare(const Camera& camera);
	void BeginBatch();
	void AddQuad(const Vec3(&pts)[4], const Vec4& color);
	void EndBatch();*/

private:
	void ReserveVertexBuffer(uint vertexCount);
	void ReserveIndexBuffer(uint indexCount);

	ID3D11DeviceContext* deviceContext;
	Shader shaderSimple, shaderColor, shaderArea;
	ID3D11Buffer* vsGlobals;
	ID3D11Buffer* psGlobals;
	MeshPtr meshes[4];
	ID3D11Buffer* vb;
	ID3D11Buffer* ib;
	uint vbSize, ibSize;
	Vec3 playerPos;

	/*VB vb;
	uint vb_size;
	vector<VColor> verts;
	Matrix mat_view_proj;
	bool batch;*/
};
