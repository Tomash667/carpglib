#include "EnginePch.h"
#include "EngineCore.h"
#include "ParticleShader.h"
#include "ParticleSystem.h"
#include "Render.h"
#include "CameraBase.h"
#include "Texture.h"
#include "DirectX.h"

//=================================================================================================
ParticleShader::ParticleShader() : effect(nullptr), vb(nullptr)
{
}

//=================================================================================================
void ParticleShader::OnInit()
{
	effect = app::render->CompileShader("particle.fx");

	techParticle = effect->GetTechniqueByName("particle");
	techTrail = effect->GetTechniqueByName("trail");
	assert(techParticle && techTrail);

	hMatCombined = effect->GetParameterByName(nullptr, "matCombined");
	hTex = effect->GetParameterByName(nullptr, "tex0");
	assert(hMatCombined && hTex);
}

//=================================================================================================
void ParticleShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
	SafeRelease(vb);
}

//=================================================================================================
void ParticleShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void ParticleShader::OnRelease()
{
	SafeRelease(effect);
	SafeRelease(vb);
}

//=================================================================================================
void ParticleShader::Prepare(CameraBase& camera)
{
	mat_view_proj = camera.mat_view_proj;
	mat_view_inv = camera.mat_view_inv;

	app::render->SetAlphaTest(false);
	app::render->SetAlphaBlend(true);
	app::render->SetNoCulling(true);
	app::render->SetNoZWrite(true);
}

//=================================================================================================
void ParticleShader::DrawParticles(const vector<ParticleEmitter*>& pes)
{
	IDirect3DDevice9* device = app::render->GetDevice();

	V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(VDI_PARTICLE)));

	uint passes;
	V(effect->SetTechnique(techParticle));
	V(effect->SetMatrix(hMatCombined, (D3DXMATRIX*)&mat_view_proj));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	for(vector<ParticleEmitter*>::const_iterator it = pes.begin(), end = pes.end(); it != end; ++it)
	{
		const ParticleEmitter& pe = **it;

		// stwórz vertex buffer na cz¹steczki jeœli nie ma wystarczaj¹co du¿ego
		if(!vb || particle_count < pe.alive)
		{
			if(vb)
				vb->Release();
			V(device->CreateVertexBuffer(sizeof(VParticle) * pe.alive * 6, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vb, nullptr));
			particle_count = pe.alive;
		}
		V(device->SetStreamSource(0, vb, 0, sizeof(VParticle)));

		// wype³nij vertex buffer
		VParticle* v;
		V(vb->Lock(0, sizeof(VParticle) * pe.alive * 6, (void**)&v, D3DLOCK_DISCARD));
		int idx = 0;
		for(vector<Particle>::const_iterator it2 = pe.particles.begin(), end2 = pe.particles.end(); it2 != end2; ++it2)
		{
			const Particle& p = *it2;
			if(!p.exists)
				continue;

			mat_view_inv._41 = p.pos.x;
			mat_view_inv._42 = p.pos.y;
			mat_view_inv._43 = p.pos.z;
			Matrix m1 = Matrix::Scale(pe.GetScale(p)) * mat_view_inv;

			const Vec4 color(1.f, 1.f, 1.f, pe.GetAlpha(p));

			v[idx].pos = Vec3::Transform(Vec3(-1, -1, 0), m1);
			v[idx].tex = Vec2(0, 0);
			v[idx].color = color;

			v[idx + 1].pos = Vec3::Transform(Vec3(-1, 1, 0), m1);
			v[idx + 1].tex = Vec2(0, 1);
			v[idx + 1].color = color;

			v[idx + 2].pos = Vec3::Transform(Vec3(1, -1, 0), m1);
			v[idx + 2].tex = Vec2(1, 0);
			v[idx + 2].color = color;

			v[idx + 3] = v[idx + 1];

			v[idx + 4].pos = Vec3::Transform(Vec3(1, 1, 0), m1);
			v[idx + 4].tex = Vec2(1, 1);
			v[idx + 4].color = color;

			v[idx + 5] = v[idx + 2];

			idx += 6;
		}

		V(vb->Unlock());

		switch(pe.mode)
		{
		case 0:
			V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
			V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
			V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
			break;
		case 1:
			V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
			V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
			V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));
			break;
		case 2:
			V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT));
			V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
			V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));
			break;
		default:
			assert(0);
			break;
		}

		V(effect->SetTexture(hTex, pe.tex->tex));
		V(effect->CommitChanges());

		V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, pe.alive * 2));
	}

	V(effect->EndPass());
	V(effect->End());

	V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
	V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
}

//=================================================================================================
void ParticleShader::DrawTrailParticles(const vector<TrailParticleEmitter*>& tpes)
{
	IDirect3DDevice9* device = app::render->GetDevice();

	V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(VDI_COLOR)));

	uint passes;
	V(effect->SetTechnique(techTrail));
	V(effect->SetMatrix(hMatCombined, (D3DXMATRIX*)&mat_view_proj));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	VColor v[4];

	for(vector<TrailParticleEmitter*>::const_iterator it = tpes.begin(), end = tpes.end(); it != end; ++it)
	{
		const TrailParticleEmitter& tp = **it;

		if(tp.alive < 2)
			continue;

		int id = tp.first;
		const TrailParticle* prev = &tp.parts[id];
		id = prev->next;

		while(id != -1)
		{
			const TrailParticle& p = tp.parts[id];

			v[0].pos = prev->pt1;
			v[1].pos = prev->pt2;
			v[2].pos = p.pt1;
			v[3].pos = p.pt2;

			v[0].color = Vec4::Lerp(tp.color1, tp.color2, 1.f - prev->t / tp.fade);
			v[1].color = v[0].color;
			v[2].color = Vec4::Lerp(tp.color1, tp.color2, 1.f - p.t / tp.fade);
			v[3].color = v[2].color;

			V(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VColor)));

			prev = &p;
			id = prev->next;
		}
	}

	V(effect->EndPass());
	V(effect->End());
}
