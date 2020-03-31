#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class TerrainShader : public ShaderHandler
{
public:
	TerrainShader();
	void OnInit() override;
	void OnRelease() override;
	void Draw(Scene* scene, Camera* camera, Terrain* terrain, const vector<uint>& parts);

private:
	ID3D11DeviceContext* device_context;
	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vs_globals;
	ID3D11Buffer* ps_globals;
	ID3D11SamplerState* samplers[6];
};
