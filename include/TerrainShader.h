#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class TerrainShader : public ShaderHandler
{
public:
	TerrainShader();
	cstring GetName() const override { return "terrain"; }
	void OnInit() override;
	void OnRelease() override;
	void Prepare(Scene* scene, Camera* camera);
	void Draw(Terrain* terrain, const vector<uint>& parts);

private:
	ID3D11DeviceContext* deviceContext;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vsGlobals;
	ID3D11Buffer* psGlobals;
	ID3D11SamplerState* sampler;
	Camera* camera;
};
