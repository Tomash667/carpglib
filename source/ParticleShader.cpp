#include "Pch.h"
#include "ParticleShader.h"

#include "Camera.h"
#include "DirectX.h"
#include "ParticleSystem.h"
#include "Render.h"
#include "Texture.h"

struct VsGlobals
{
	Matrix matCombined;
};

//=================================================================================================
ParticleShader::ParticleShader() : deviceContext(app::render->GetDeviceContext()), vertexShader(nullptr), pixelShader(nullptr), layout(nullptr),
vsGlobals(nullptr), sampler(nullptr), vb(nullptr), texEmpty(nullptr), particleCount(0)
{
}

//=================================================================================================
void ParticleShader::OnInit()
{
	app::render->CreateShader("particle.hlsl", VDI_PARTICLE, vertexShader, pixelShader, layout);
	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals));
	sampler = app::render->CreateSampler();
	texEmpty = app::render->CreateImmutableTexture(Int2(1, 1), &Color::White);

	vBillboard[0].pos = Vec3(-1, -1, 0);
	vBillboard[0].tex = Vec2(0, 0);
	vBillboard[0].color = Vec4(1.f, 1.f, 1.f, 1.f);
	vBillboard[1].pos = Vec3(-1, 1, 0);
	vBillboard[1].tex = Vec2(0, 1);
	vBillboard[1].color = Vec4(1.f, 1.f, 1.f, 1.f);
	vBillboard[2].pos = Vec3(1, -1, 0);
	vBillboard[2].tex = Vec2(1, 0);
	vBillboard[2].color = Vec4(1.f, 1.f, 1.f, 1.f);
	vBillboard[3] = vBillboard[2];
	vBillboard[4] = vBillboard[1];
	vBillboard[5].pos = Vec3(1, 1, 0);
	vBillboard[5].tex = Vec2(1, 1);
	vBillboard[5].color = Vec4(1.f, 1.f, 1.f, 1.f);

	billboardExt[0] = Vec3(-1, -1, 0);
	billboardExt[1] = Vec3(-1, 1, 0);
	billboardExt[2] = Vec3(1, -1, 0);
	billboardExt[3] = Vec3(1, -1, 0);
	billboardExt[4] = Vec3(-1, 1, 0);
	billboardExt[5] = Vec3(1, 1, 0);
}

//=================================================================================================
void ParticleShader::OnRelease()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShader);
	SafeRelease(layout);
	SafeRelease(vsGlobals);
	SafeRelease(sampler);
	SafeRelease(vb);
	SafeRelease(texEmpty);
	particleCount = 0;
}

//=================================================================================================
void ParticleShader::Prepare(Camera& camera)
{
	matViewInv = camera.mat_view_inv;
	camPos = camera.from;

	app::render->SetDepthState(Render::DEPTH_READ);
	app::render->SetRasterState(Render::RASTER_NO_CULLING);

	// setup shader
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, &sampler);
	uint stride = sizeof(VParticle), offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	deviceContext->IASetInputLayout(layout);
	ReserveVertexBuffer(1);

	// vertex shader constants
	ResourceLock lock(vsGlobals);
	lock.Get<VsGlobals>()->matCombined = camera.mat_view_proj.Transpose();

	lastTex = (Texture*)0xFEFEFEFE;
}

//=================================================================================================
void ParticleShader::DrawBillboards(const vector<Billboard>& billboards)
{
	app::render->SetBlendState(Render::BLEND_ADD);

	for(vector<Billboard>::const_iterator it = billboards.begin(), end = billboards.end(); it != end; ++it)
	{
		matViewInv._41 = it->pos.x;
		matViewInv._42 = it->pos.y;
		matViewInv._43 = it->pos.z;
		Matrix m1 = Matrix::Scale(it->size) * matViewInv;

		for(int i = 0; i < 6; ++i)
			vBillboard[i].pos = Vec3::Transform(billboardExt[i], m1);

		// fill vertex buffer
		{
			ResourceLock lock(vb);
			memcpy(lock.Get(), vBillboard, sizeof(VParticle) * 6);
		}

		if(lastTex != it->tex)
		{
			lastTex = it->tex;
			deviceContext->PSSetShaderResources(0, 1, &it->tex->tex);
		}

		deviceContext->Draw(6, 0);
	}
}

//=================================================================================================
void ParticleShader::DrawParticles(const vector<ParticleEmitter*>& pes)
{
	for(vector<ParticleEmitter*>::const_iterator it = pes.begin(), end = pes.end(); it != end; ++it)
	{
		const ParticleEmitter& pe = **it;

		ReserveVertexBuffer(pe.alive);

		// fill vertex buffer
		{
			ResourceLock lock(vb);
			VParticle* v = lock.Get<VParticle>();
			int idx = 0;
			for(const ParticleEmitter::Particle& p : pe.particles)
			{
				if(!p.exists)
					continue;

				matViewInv._41 = p.pos.x;
				matViewInv._42 = p.pos.y;
				matViewInv._43 = p.pos.z;
				Matrix m1 = Matrix::Scale(pe.GetScale(p)) * matViewInv;

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
		}

		// set blending
		app::render->SetBlendState((Render::BlendState)(pe.mode + 1));

		// set texture
		if(lastTex != pe.tex)
		{
			lastTex = pe.tex;
			deviceContext->PSSetShaderResources(0, 1, &pe.tex->tex);
		}

		// draw
		deviceContext->Draw(pe.alive * 6, 0);
	}
}

//=================================================================================================
void ParticleShader::DrawTrailParticles(const vector<TrailParticleEmitter*>& tpes)
{
	app::render->SetBlendState(Render::BLEND_ADD_ONE);

	for(vector<TrailParticleEmitter*>::const_iterator it = tpes.begin(), end = tpes.end(); it != end; ++it)
	{
		const TrailParticleEmitter& tp = **it;

		if(tp.alive < 2)
			continue;

		uint count = tp.alive - 1;
		ReserveVertexBuffer(count);

		// fill vertex buffer
		{
			int id = tp.first;
			const TrailParticleEmitter::Particle* prev = &tp.parts[id];
			const float width = tp.width / 2;
			id = prev->next;
			ResourceLock lock(vb);
			VParticle* v = lock.Get<VParticle>();
			int idx = 0;
			while(id != -1)
			{
				const TrailParticleEmitter::Particle& p = tp.parts[id];

				Vec3 line_dir = p.pt - prev->pt;
				Vec3 quad_normal = camPos - (p.pt + prev->pt) / 2;
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
		}

		// set texture
		if(tp.tex != lastTex)
		{
			lastTex = tp.tex;
			deviceContext->PSSetShaderResources(0, 1, &(lastTex ? lastTex->tex : texEmpty));
		}

		// draw
		deviceContext->Draw(count * 6, 0);
	}
}

//=================================================================================================
void ParticleShader::ReserveVertexBuffer(uint count)
{
	if(!vb || particleCount < count)
	{
		if(vb)
			vb->Release();

		// create vertex buffer
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.ByteWidth = sizeof(VParticle) * 6 * count;
		bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;
		bufDesc.StructureByteStride = 0;

		V(app::render->GetDevice()->CreateBuffer(&bufDesc, nullptr, &vb));
		SetDebugName(vb, "ParticleVb");

		uint stride = sizeof(VParticle),
			offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

		particleCount = count;
	}
}
