#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
class ParticleShader : public ShaderHandler
{
public:
	ParticleShader();
	cstring GetName() const override { return "particle"; }
	void OnInit() override;
	void OnRelease() override;
	/*void Begin(Camera& camera);
	void End();
	void DrawBillboards(const vector<Billboard>& billboards);
	void DrawParticles(const vector<ParticleEmitter*>& pes);
	void DrawTrailParticles(const vector<TrailParticleEmitter*>& tpes);*/

	/*ID3DXEffect* effect;
	D3DXHANDLE tech;
	D3DXHANDLE hMatCombined, hTex;*/

private:
	void ReserveVertexBuffer(uint size);

	//IDirect3DDevice9* device;
	//VB vb;
	VParticle billboard_v[4];
	Vec3 billboard_ext[4];
	Matrix mat_view_proj, mat_view_inv;
	TEX tex_empty;
	Vec3 cam_pos;
	uint particle_count;
	Texture* last_tex;
	int last_mode;
};
