#include "Pch.h"
#include "SceneManager.h"
#include "Render.h"
#include "Scene.h"
#include "SceneNode.h"
//#include "SuperShader.h"
#include "Camera.h"
#include "DirectX.h"

SceneManager* app::scene_mgr;

//=================================================================================================
SceneManager::SceneManager() : use_lighting(true), use_fog(true), use_normalmap(true), use_specularmap(true)
{
}

//=================================================================================================
void SceneManager::Init()
{
	/*device = app::render->GetDevice();

	super_shader = new SuperShader;
	app::render->RegisterShader(super_shader);*/
}

//=================================================================================================
void SceneManager::Draw(Scene* scene, Camera* camera, RenderTarget* target)
{
	/*assert(scene && camera && target);

	this->scene = scene;
	this->camera = camera;

	batch.Clear();
	batch.camera = camera;
	batch.gather_lights = use_lighting && !scene->use_light_dir;
	scene->ListNodes(batch);
	batch.Process();

	IDirect3DDevice9* device = app::render->GetDevice();

	app::render->SetTarget(target);
	V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0, 1.f, 0));
	V(device->BeginScene());

	if(!batch.node_groups.empty())
		DrawSceneNodes(batch.nodes, batch.node_groups);

	if(!batch.alpha_nodes.empty())
		DrawAlphaSceneNodes(batch.alpha_nodes);

	V(device->EndScene());
	app::render->SetTarget(nullptr);*/
}

//=================================================================================================
void SceneManager::DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<SceneNodeGroup>& groups)
{
	/*app::render->SetAlphaBlend(false);

	const bool use_fog = this->use_fog && use_lighting;

	// setup effect
	ID3DXEffect* effect = super_shader->GetEffect();
	if(use_lighting)
	{
		Vec4 value = scene->ambient_color;
		V(effect->SetVector(super_shader->hAmbientColor, (D3DXVECTOR4*)&value));
		if(use_fog)
		{
			value = scene->fog_color;
			V(effect->SetVector(super_shader->hFogColor, (D3DXVECTOR4*)&value));
			value = scene->GetFogParams();
			V(effect->SetVector(super_shader->hFogParams, (D3DXVECTOR4*)&value));
		}
		if(scene->use_light_dir)
		{
			value = Vec4(scene->light_dir, 1);
			V(effect->SetVector(super_shader->hLightDir, (D3DXVECTOR4*)&value));
			value = scene->light_color;
			V(effect->SetVector(super_shader->hLightColor, (D3DXVECTOR4*)&value));
		}
	}
	else
		V(effect->SetVector(super_shader->hAmbientColor, (D3DXVECTOR4*)&Vec4::One));
	V(effect->SetVector(super_shader->hCameraPos, (D3DXVECTOR4*)&camera->from));

	// for each group
	const Mesh* prev_mesh = nullptr;
	for(const SceneNodeGroup& group : groups)
	{
		const bool animated = IsSet(group.flags, SceneNode::F_ANIMATED);
		const bool normal_map = IsSet(group.flags, SceneNode::F_NORMAL_MAP);
		const bool specular_map = IsSet(group.flags, SceneNode::F_SPECULAR_MAP);
		const bool use_lighting = this->use_lighting && !IsSet(group.flags, SceneNode::F_NO_LIGHTING);

		effect = super_shader->GetShader(super_shader->GetShaderId(
			animated,
			IsSet(group.flags, SceneNode::F_TANGENTS),
			use_fog,
			specular_map,
			normal_map,
			use_lighting && !scene->use_light_dir,
			use_lighting && scene->use_light_dir));
		D3DXHANDLE tech;
		uint passes;
		V(effect->FindNextValidTechnique(nullptr, &tech));
		V(effect->SetTechnique(tech));
		V(effect->Begin(&passes, 0));
		V(effect->BeginPass(0));

		app::render->SetNoZWrite(IsSet(group.flags, SceneNode::F_NO_ZWRITE));
		app::render->SetNoCulling(IsSet(group.flags, SceneNode::F_NO_CULLING));
		app::render->SetAlphaTest(IsSet(group.flags, SceneNode::F_ALPHA_TEST));

		// for each node in group
		for(auto it = nodes.begin() + group.start, end = nodes.begin() + group.end + 1; it != end; ++it)
		{
			const SceneNode* node = *it;
			const Mesh& mesh = *node->mesh;

			// set shader params
			Matrix m1;
			if(node->type == SceneNode::BILLBOARD)
				m1 = node->mat.Inverse() * camera->mat_view_proj;
			else
				m1 = node->mat * camera->mat_view_proj;
			V(effect->SetMatrix(super_shader->hMatCombined, (D3DXMATRIX*)&m1));
			V(effect->SetMatrix(super_shader->hMatWorld, (D3DXMATRIX*)&node->mat));
			V(effect->SetVector(super_shader->hTint, (D3DXVECTOR4*)&node->tint));
			if(animated)
			{
				const vector<Matrix>& mat_bones = node->mesh_inst->mat_bones;
				V(effect->SetMatrixArray(super_shader->hMatBones, (D3DXMATRIX*)mat_bones.data(), mat_bones.size()));
			}

			// set mesh
			if(prev_mesh != &mesh)
			{
				V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(mesh.vertex_decl)));
				V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
				V(device->SetIndices(mesh.ib));
				prev_mesh = &mesh;
			}

			// set lights
			if(use_lighting && !scene->use_light_dir)
				super_shader->ApplyLights(node->lights);

			// draw
			if(!IsSet(node->subs, SceneNode::SPLIT_INDEX))
			{
				for(int i = 0; i < mesh.head.n_subs; ++i)
				{
					if(!IsSet(node->subs, 1 << i))
						continue;

					const Mesh::Submesh& sub = mesh.subs[i];

					// texture
					V(effect->SetTexture(super_shader->hTexDiffuse, mesh.GetTexture(i, node->tex_override)));
					if(normal_map)
					{
						TEX tex = sub.tex_normal ? sub.tex_normal->tex : super_shader->tex_empty_normal_map;
						V(effect->SetTexture(super_shader->hTexNormal, tex));
					}
					if(specular_map)
					{
						TEX tex = sub.tex_specular ? sub.tex_specular->tex : super_shader->tex_empty_specular_map;
						V(effect->SetTexture(super_shader->hTexSpecular, tex));
					}

					// material light settings
					V(effect->SetVector(super_shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
					V(effect->SetFloat(super_shader->hSpecularIntensity, sub.specular_intensity));
					V(effect->SetFloat(super_shader->hSpecularHardness, (float)sub.specular_hardness));

					V(effect->CommitChanges());
					V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
				}
			}
			else
			{
				int index = (node->subs & ~SceneNode::SPLIT_INDEX);
				const Mesh::Submesh& sub = mesh.subs[index];

				// texture
				V(effect->SetTexture(super_shader->hTexDiffuse, mesh.GetTexture(index, node->tex_override)));
				if(normal_map)
				{
					TEX tex = sub.tex_normal ? sub.tex_normal->tex : super_shader->tex_empty_normal_map;
					V(effect->SetTexture(super_shader->hTexNormal, tex));
				}
				if(specular_map)
				{
					TEX tex = sub.tex_specular ? sub.tex_specular->tex : super_shader->tex_empty_specular_map;
					V(effect->SetTexture(super_shader->hTexSpecular, tex));
				}

				// material light settings
				V(effect->SetVector(super_shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
				V(effect->SetFloat(super_shader->hSpecularIntensity, sub.specular_intensity));
				V(effect->SetFloat(super_shader->hSpecularHardness, (float)sub.specular_hardness));

				V(effect->CommitChanges());
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
			}
		}

		V(effect->EndPass());
		V(effect->End());
	}*/
}

//=================================================================================================
void SceneManager::DrawAlphaSceneNodes(const vector<SceneNode*>& nodes)
{
	/*app::render->SetAlphaBlend(true);
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));

	const bool use_fog = this->use_fog && use_lighting;

	// setup effect
	ID3DXEffect* effect = super_shader->GetEffect();
	if(use_lighting)
	{
		Vec4 value = scene->ambient_color;
		V(effect->SetVector(super_shader->hAmbientColor, (D3DXVECTOR4*)&value));
		if(use_fog)
		{
			value = scene->fog_color;
			V(effect->SetVector(super_shader->hFogColor, (D3DXVECTOR4*)&value));
			value = scene->GetFogParams();
			V(effect->SetVector(super_shader->hFogParams, (D3DXVECTOR4*)&value));
		}
		if(scene->use_light_dir)
		{
			value = Vec4(scene->light_dir, 1);
			V(effect->SetVector(super_shader->hLightDir, (D3DXVECTOR4*)&value));
			value = scene->light_color;
			V(effect->SetVector(super_shader->hLightColor, (D3DXVECTOR4*)&value));
		}
	}
	else
		V(effect->SetVector(super_shader->hAmbientColor, (D3DXVECTOR4*)&Vec4::One));
	V(effect->SetVector(super_shader->hCameraPos, (D3DXVECTOR4*)&camera->from));

	// for each group
	const Mesh* prev_mesh = nullptr;
	uint last_id = -1;
	bool open = false;
	for(const SceneNode* node : nodes)
	{
		const bool animated = IsSet(node->flags, SceneNode::F_ANIMATED);
		const bool normal_map = IsSet(node->flags, SceneNode::F_NORMAL_MAP);
		const bool specular_map = IsSet(node->flags, SceneNode::F_SPECULAR_MAP);
		const bool use_lighting = this->use_lighting && !IsSet(node->flags, SceneNode::F_NO_LIGHTING);

		uint id = super_shader->GetShaderId(
			animated,
			IsSet(node->flags, SceneNode::F_TANGENTS),
			use_fog,
			specular_map,
			normal_map,
			use_lighting && !scene->use_light_dir,
			use_lighting && scene->use_light_dir);
		if(id != last_id)
		{
			if(open)
			{
				effect->EndPass();
				effect->End();
			}

			app::render->SetNoZWrite(IsSet(node->flags, SceneNode::F_NO_ZWRITE));
			app::render->SetNoCulling(IsSet(node->flags, SceneNode::F_NO_CULLING));
			app::render->SetAlphaTest(IsSet(node->flags, SceneNode::F_ALPHA_TEST));

			effect = super_shader->GetShader(id);
			D3DXHANDLE tech;
			uint passes;
			V(effect->FindNextValidTechnique(nullptr, &tech));
			V(effect->SetTechnique(tech));
			effect->Begin(&passes, 0);
			effect->BeginPass(0);

			open = true;
		}

		const Mesh& mesh = *node->mesh;

		// shader params
		Matrix m1;
		if(node->type == SceneNode::BILLBOARD)
			m1 = node->mat.Inverse() * camera->mat_view_proj;
		else
			m1 = node->mat * camera->mat_view_proj;
		V(effect->SetMatrix(super_shader->hMatCombined, (D3DXMATRIX*)&m1));
		V(effect->SetMatrix(super_shader->hMatWorld, (D3DXMATRIX*)&node->mat));
		V(effect->SetVector(super_shader->hTint, (D3DXVECTOR4*)&node->tint));
		if(animated)
		{
			const vector<Matrix>& mat_bones = node->mesh_inst->mat_bones;
			V(effect->SetMatrixArray(super_shader->hMatBones, (D3DXMATRIX*)mat_bones.data(), mat_bones.size()));
		}

		// mesh
		if(prev_mesh != &mesh)
		{
			V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(mesh.vertex_decl)));
			V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
			V(device->SetIndices(mesh.ib));
			prev_mesh = &mesh;
		}

		// set lights
		if(use_lighting && !scene->use_light_dir)
			super_shader->ApplyLights(node->lights);

		// drawing
		assert(!IsSet(node->subs, SceneNode::SPLIT_INDEX)); // yagni
		for(int i = 0; i < mesh.head.n_subs; ++i)
		{
			if(!IsSet(node->subs, 1 << i))
				continue;

			const Mesh::Submesh& sub = mesh.subs[i];

			// texture
			V(effect->SetTexture(super_shader->hTexDiffuse, mesh.GetTexture(i, node->tex_override)));
			if(normal_map)
			{
				TEX tex = sub.tex_normal ? sub.tex_normal->tex : super_shader->tex_empty_normal_map;
				V(effect->SetTexture(super_shader->hTexNormal, tex));
			}
			if(specular_map)
			{
				TEX tex = sub.tex_specular ? sub.tex_specular->tex : super_shader->tex_empty_specular_map;
				V(effect->SetTexture(super_shader->hTexSpecular, tex));
			}

			// material light settings
			V(effect->SetVector(super_shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
			V(effect->SetFloat(super_shader->hSpecularIntensity, sub.specular_intensity));
			V(effect->SetFloat(super_shader->hSpecularHardness, (float)sub.specular_hardness));

			V(effect->CommitChanges());
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
		}
	}

	if(open)
	{
		effect->EndPass();
		effect->End();
	}

	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));*/
}
