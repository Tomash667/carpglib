#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class GrassShader : public ShaderHandler
{
public:
	GrassShader();
	cstring GetName() const override { return "grass"; }
	void OnInit() override;
	void OnRelease() override;
	void Prepare(Scene* scene, Camera* camera);
	void Draw(Mesh* mesh, const vector<const vector<Matrix>*>& patches, uint count);

private:
	void ReserveVertexBuffer(uint vertexCount);

	ID3D11DeviceContext* deviceContext;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vsGlobals;
	ID3D11Buffer* psGlobals;
	ID3D11Buffer* vb;
	uint vbSize;
};
