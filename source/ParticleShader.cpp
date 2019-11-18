#include "EnginePch.h"
#include "EngineCore.h"
#include "ParticleShader.h"
#include "ParticleSystem.h"
#include "Render.h"
#include "CameraBase.h"
#include "Texture.h"
#include "DirectX.h"

//=================================================================================================
ParticleShader::ParticleShader() : device(app::render->GetDevice()), effect(nullptr), vb(nullptr)
{
}

//=================================================================================================
void ParticleShader::OnInit()
{
	effect = app::render->CompileShader("particle.fx");

	tech = effect->GetTechniqueByName("particle");
	assert(tech);

	hMatCombined = effect->GetParameterByName(nullptr, "matCombined");
	hTex = effect->GetParameterByName(nullptr, "tex0");
	assert(hMatCombined && hTex);

	if(!tex_empty)
		tex_empty = app::render->CreateTexture(Int2(1, 1), &Color::White);
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
void ParticleShader::Begin(CameraBase& camera)
{
	mat_view_proj = camera.mat_view_proj;
	mat_view_inv = camera.mat_view_inv;
	cam_pos = camera.from;

	app::render->SetAlphaTest(false);
	app::render->SetAlphaBlend(true);
	app::render->SetNoCulling(true);
	app::render->SetNoZWrite(true);

	V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(VDI_PARTICLE)));
	if(vb)
		V(device->SetStreamSource(0, vb, 0, sizeof(VParticle)));

	V(effect->SetTechnique(tech));
	V(effect->SetMatrix(hMatCombined, (D3DXMATRIX*)&mat_view_proj));

	last_mode = 0;
}

//=================================================================================================
void ParticleShader::End()
{
	if(last_mode != 0)
	{
		V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
		V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
		V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	}
}

//=================================================================================================
void ParticleShader::DrawParticles(const vector<ParticleEmitter*>& pes)
{
	Texture* last_tex = nullptr;
	uint passes;
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	for(vector<ParticleEmitter*>::const_iterator it = pes.begin(), end = pes.end(); it != end; ++it)
	{
		const ParticleEmitter& pe = **it;

		ReserveVertexBuffer(pe.alive);

		// fill vertex buffer
		VParticle* v;
		V(vb->Lock(0, sizeof(VParticle) * pe.alive * 6, (void**)&v, D3DLOCK_DISCARD));
		int idx = 0;
		for(const ParticleEmitter::Particle& p : pe.particles)
		{
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

		// set blending
		if(last_mode != pe.mode)
		{
			last_mode = pe.mode;
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
		}

		// set texture
		if(last_tex != pe.tex)
		{
			last_tex = pe.tex;
			V(effect->SetTexture(hTex, pe.tex->tex));
			V(effect->CommitChanges());
		}

		// draw
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
	if(last_mode != 1)
	{
		last_mode = 1;
		V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
		V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
		V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));
	}

	Texture* last_tex = nullptr;
	uint passes;
	V(effect->SetTexture(hTex, tex_empty));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	const Vec3 shift[2] = {
		Vec3(0,-1,0),
		Vec3(0, 1,0)
	};
	VParticle v[4];

	for(vector<TrailParticleEmitter*>::const_iterator it = tpes.begin(), end = tpes.end(); it != end; ++it)
	{
		const TrailParticleEmitter& tp = **it;

		if(tp.alive < 2)
			continue;

		uint count = tp.alive - 1;
		ReserveVertexBuffer(count);

		// fill vertex buffer
		int id = tp.first;
		const TrailParticleEmitter::Particle* prev = &tp.parts[id];
		const float width = tp.width / 2;
		id = prev->next;
		VParticle* v;
		V(vb->Lock(0, sizeof(VParticle) * count * 6, (void**)&v, D3DLOCK_DISCARD));
		int idx = 0;
		while(id != -1)
		{
			const TrailParticleEmitter::Particle& p = tp.parts[id];

			Vec3 line_dir = p.pt - prev->pt;
			Vec3 quad_normal = cam_pos - (p.pt + prev->pt) / 2;
			Vec3 extrude_dir = line_dir.Cross(quad_normal).Normalize();

			v[idx].pos = prev->pt + extrude_dir * width;
			v[idx + 1].pos = prev->pt - extrude_dir * width;
			v[idx + 2].pos = p.pt + extrude_dir * width;
			v[idx + 3].pos = p.pt - extrude_dir * width;

			v[idx].color = Vec4::Lerp(tp.color1, tp.color2, 1.f - prev->t / tp.fade);
			v[idx + 1].color = v[idx].color;
			v[idx + 2].color = Vec4::Lerp(tp.color1, tp.color2, 1.f - p.t / tp.fade);
			v[idx + 3].color = v[idx + 2].color;

			v[idx].tex = Vec2(0, 0);
			v[idx + 1].tex = Vec2(0, 1);
			v[idx + 2].tex = Vec2(1, 0);
			v[idx + 3].tex = Vec2(1, 1);

			v[idx + 4] = v[idx + 1];
			v[idx + 5] = v[idx + 2];

			prev = &p;
			id = prev->next;
			idx += 6;
		}
		V(vb->Unlock());

		// set texture
		if(tp.tex != last_tex)
		{
			last_tex = tp.tex;
			V(effect->SetTexture(hTex, tp.tex ? tp.tex->tex : tex_empty));
			V(effect->CommitChanges());
		}

		// draw
		V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, count * 2));
	}

	V(effect->EndPass());
	V(effect->End());
}

//=================================================================================================
void ParticleShader::ReserveVertexBuffer(uint size)
{
	if(!vb || particle_count < size)
	{
		if(vb)
			vb->Release();
		V(device->CreateVertexBuffer(sizeof(VParticle) * size * 6, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vb, nullptr));
		V(device->SetStreamSource(0, vb, 0, sizeof(VParticle)));
		particle_count = size;
	}
}