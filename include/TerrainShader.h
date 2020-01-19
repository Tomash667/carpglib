#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class TerrainShader : public ShaderHandler
{
public:
	TerrainShader();
	~TerrainShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	void Draw(Scene* scene, Camera* camera, Terrain* terrain, const vector<uint>& parts);

private:
	IDirect3DDevice9* device;
	IDirect3DVertexDeclaration9* vertex_decl;
	ID3DXEffect* effect;
	D3DXHANDLE tech;
	D3DXHANDLE h_mat, h_world, h_tex_blend, h_tex[5], h_color_ambient, h_color_diffuse, h_light_dir, h_fog_color, h_fog_params;
};
