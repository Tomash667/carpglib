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
	void DrawDebugNodes(const vector<DebugSceneNode*>& nodes);
	/*void Prepare(const Camera& camera);
	void BeginBatch();
	void AddQuad(const Vec3(&pts)[4], const Vec4& color);
	void EndBatch();*/

private:
	ID3D11DeviceContext* deviceContext;
	Shader shaderSimple, shaderColor, shaderArea;
	ID3D11Buffer* vsGlobals;
	ID3D11Buffer* psGlobals;

	/*VB vb;
	uint vb_size;
	vector<VColor> verts;
	Matrix mat_view_proj;
	bool batch;*/
};
