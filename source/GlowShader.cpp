#include "Pch.h"
#include "GlowShader.h"

#include "DirectX.h"
#include "Mesh.h"
#include "Render.h"

struct VsGlobals
{
	Matrix matCombined;
	Matrix matBones[Mesh::MAX_BONES];
};

struct PsGlobals
{
	Vec4 color;
};

//=================================================================================================
GlowShader::GlowShader() : vertexShaderMesh(nullptr), vertexShaderAni(nullptr), pixelShader(nullptr), layoutMesh(nullptr), layoutMeshTangent(nullptr),
layoutAni(nullptr), layoutAniTangent(nullptr), vsGlobals(nullptr), psGlobals(nullptr), sampler(nullptr)
{
}

//=================================================================================================
void GlowShader::OnInit()
{
	ID3DBlob* vsBlob;

	vertexShaderMesh = app::render->CreateVertexShader("glow.hlsl", "VsMesh", &vsBlob);
	layoutMesh = app::render->CreateInputLayout(VDI_DEFAULT, vsBlob, "GlowMeshLayout");
	layoutMeshTangent = app::render->CreateInputLayout(VDI_TANGENT, vsBlob, "GlowMeshTangentLayout");
	vsBlob->Release();

	vertexShaderAni = app::render->CreateVertexShader("glow.hlsl", "VsAni", &vsBlob);
	layoutAni = app::render->CreateInputLayout(VDI_ANIMATED, vsBlob, "GlowAniLayout");
	layoutAniTangent = app::render->CreateInputLayout(VDI_ANIMATED_TANGENT, vsBlob, "GlowAniTangentLayout");
	vsBlob->Release();

	pixelShader = app::render->CreatePixelShader("glow.hlsl");

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "GlowVsGlobals");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "GlowPsGlobals");
	sampler = app::render->CreateSampler();
}

//=================================================================================================
void GlowShader::OnRelease()
{
	SafeRelease(vertexShaderMesh);
	SafeRelease(vertexShaderAni);
	SafeRelease(pixelShader);
	SafeRelease(layoutMesh);
	SafeRelease(layoutMeshTangent);
	SafeRelease(layoutAni);
	SafeRelease(layoutAniTangent);
	SafeRelease(vsGlobals);
	SafeRelease(psGlobals);
	SafeRelease(sampler);
}

void GlowShader::Begin()
{
	// ustaw flagi renderowania
	app::render->SetAlphaBlend(false);
	app::render->SetDepthState(Render::DEPTH_USE_STENCIL);
	app::render->SetNoCulling(false);

	// ustaw render target
	/*SURFACE render_surface;
	if(!render->IsMultisamplingEnabled())
		V(postfx_shader->tex[0]->GetSurfaceLevel(0, &render_surface));
	else
		render_surface = postfx_shader->surf[0];
	V(device->SetRenderTarget(0, render_surface));
	V(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 0, 0));
	V(device->BeginScene());

	// renderuj wszystkie obiekty
	int prev_mode = -1;
	Vec4 glow_color;
	uint passes;

	for(const GlowNode& glow : glow_nodes)
	{
		render->SetAlphaTest(glow.alpha);

		// animowany czy nie?
		Mesh* mesh = glow.node->mesh;
		if(IsSet(glow.node->flags, SceneNode::F_ANIMATED))
		{
			if(prev_mode != 1)
			{
				if(prev_mode != -1)
				{
					V(effect->EndPass());
					V(effect->End());
				}
				prev_mode = 1;
				V(effect->SetTechnique(glow_shader->techGlowAni));
				V(effect->Begin(&passes, 0));
				V(effect->BeginPass(0));
			}
			vector<Matrix>& mat_bones = glow.node->mesh_inst->mat_bones;
			V(effect->SetMatrixArray(glow_shader->hMatBones, (D3DXMATRIX*)mat_bones.data(), mat_bones.size()));
		}
		else
		{
			if(prev_mode != 0)
			{
				if(prev_mode != -1)
				{
					V(effect->EndPass());
					V(effect->End());
				}
				prev_mode = 0;
				V(effect->SetTechnique(glow_shader->techGlowMesh));
				V(effect->Begin(&passes, 0));
				V(effect->BeginPass(0));
			}
		}

		// wybierz kolor
		if(glow.type == GlowNode::Unit)
		{
			Unit& unit = *static_cast<Unit*>(glow.ptr);
			if(pc->unit->IsEnemy(unit))
				glow_color = Vec4(1, 0, 0, 1);
			else if(pc->unit->IsFriend(unit))
				glow_color = Vec4(0, 1, 0, 1);
			else
				glow_color = Vec4(1, 1, 0, 1);
		}
		else
			glow_color = Vec4(1, 1, 1, 1);

		// ustawienia shadera
		Matrix m1 = glow.node->mat * game_level->camera.mat_view_proj;
		V(effect->SetMatrix(glow_shader->hMatCombined, (D3DXMATRIX*)&m1));
		V(effect->SetVector(glow_shader->hColor, (D3DXVECTOR4*)&glow_color));
		V(effect->CommitChanges());

		// ustawienia modelu
		V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh->vertex_decl)));
		V(device->SetStreamSource(0, mesh->vb, 0, mesh->vertex_size));
		V(device->SetIndices(mesh->ib));

		// render mesh
		if(glow.alpha)
		{
			// for glow need to set texture per submesh
			for(int i = 0; i < mesh->head.n_subs; ++i)
			{
				if(i == 0 || glow.type != GlowNode::Door)
				{
					V(effect->SetTexture(glow_shader->hTex, mesh->subs[i].tex->tex));
					V(effect->CommitChanges());
					V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
				}
			}
		}
		else
		{
			V(effect->SetTexture(glow_shader->hTex, game_res->tBlack->tex));
			V(effect->CommitChanges());
			for(int i = 0; i < mesh->head.n_subs; ++i)
			{
				if(i == 0 || glow.type != GlowNode::Door)
					V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
			}
		}
	}

	V(effect->EndPass());
	V(effect->End());
	V(device->EndScene());

	V(device->SetRenderState(D3DRS_ZENABLE, FALSE));
	V(device->SetRenderState(D3DRS_STENCILENABLE, FALSE));*/
}

void GlowShader::End()
{

}
