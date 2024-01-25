#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
class ParticleShader final : public ShaderHandler
{
public:
	ParticleShader();
	cstring GetName() const override { return "particle"; }
	void OnInit() override;
	void OnRelease() override;
	void Prepare(Camera& camera);
	void DrawBillboards(const vector<Billboard>& billboards);
	void DrawParticles(const vector<ParticleEmitter*>& pes);
	void DrawTrailParticles(const vector<TrailParticleEmitter*>& tpes);

private:
	void ReserveVertexBuffer(uint count);

	ID3D11DeviceContext* deviceContext;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vsGlobals;
	ID3D11Buffer* vb;
	TEX texEmpty;

	VParticle vBillboard[6];
	Vec3 billboardExt[6];
	Matrix matViewInv;
	Vec3 camPos;
	Texture* lastTex;
	uint particleCount;
};
