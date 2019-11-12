#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class ParticleShader : public ShaderHandler
{
public:
	ParticleShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	void Prepare(CameraBase& camera);
	void DrawParticles(const vector<ParticleEmitter*>& pes);
	void DrawTrailParticles(const vector<TrailParticleEmitter*>& tpes);

	ID3DXEffect* effect;
	D3DXHANDLE tech;
	D3DXHANDLE hMatCombined, hTex;

private:
	void ReserveVertexBuffer(uint size);

	IDirect3DDevice9* device;
	VB vb;
	Matrix mat_view_proj, mat_view_inv;
	TEX tex_empty;
	Vec3 cam_pos;
	uint particle_count;
};
