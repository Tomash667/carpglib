#include "Pch.h"
#include "SuperShader.h"

#include "Camera.h"
#include "DirectX.h"
#include "Light.h"
#include "Mesh.h"
#include "Render.h"
#include "Scene.h"
#include "SceneNode.h"

struct VsGlobals
{
	Vec3 cameraPos;
};

struct VsLocals
{
	Matrix matCombined;
	Matrix matWorld;
	Matrix matBones[Mesh::MAX_BONES];
};

struct PsGlobals
{
	Vec4 ambientColor;
	Vec4 lightColor;
	Vec4 lightDir;
	Vec4 fogColor;
	Vec4 fogParams;
};

struct PsLocals
{
	Vec4 tint;
	Lights lights;
};

struct PsMaterial
{
	Vec3 specularColor;
	float specularHardness;
	float specularIntensity;
};

//=================================================================================================
SuperShader::SuperShader() : deviceContext(app::render->GetDeviceContext()), samplerDiffuse(nullptr), samplerNormal(nullptr), samplerSpecular(nullptr),
vsGlobals(nullptr), vsLocals(nullptr), psGlobals(nullptr), psLocals(nullptr), psMaterial(nullptr), texEmptyNormalMap(nullptr), texEmptySpecularMap(nullptr)
{
}

//=================================================================================================
void SuperShader::OnInit()
{
	samplerDiffuse = app::render->CreateSampler();
	samplerNormal = app::render->CreateSampler();
	samplerSpecular = app::render->CreateSampler();

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "SuperVsGlobals");
	vsLocals = app::render->CreateConstantBuffer(sizeof(VsLocals), "SuperVsLocals");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "SuperPsGlobals");
	psLocals = app::render->CreateConstantBuffer(sizeof(PsLocals), "SuperPsLocals");
	psMaterial = app::render->CreateConstantBuffer(sizeof(PsMaterial), "SuperPsMaterial");

	texEmptyNormalMap = app::render->CreateImmutableTexture(Int2(1, 1), &Color(128, 128, 255));
	texEmptySpecularMap = app::render->CreateImmutableTexture(Int2(1, 1), &Color::None);
}

//=================================================================================================
void SuperShader::OnRelease()
{
	for(Shader& shader : shaders)
	{
		SafeRelease(shader.pixelShader);
		SafeRelease(shader.vertexShader);
		SafeRelease(shader.layout);
	}
	shaders.clear();

	SafeRelease(samplerDiffuse);
	SafeRelease(samplerNormal);
	SafeRelease(samplerSpecular);
	SafeRelease(vsGlobals);
	SafeRelease(vsLocals);
	SafeRelease(psGlobals);
	SafeRelease(psLocals);
	SafeRelease(psMaterial);
	SafeRelease(texEmptyNormalMap);
	SafeRelease(texEmptySpecularMap);
}

//=================================================================================================
uint SuperShader::GetShaderId(bool have_weights, bool have_tangents, bool animated, bool fog, bool specular_map,
	bool normal_map, bool point_light, bool dir_light) const
{
	uint id = 0;
	if(have_weights)
		id |= HAVE_WEIGHT;
	if(have_tangents)
		id |= HAVE_TANGENTS;
	if(animated)
		id |= ANIMATED;
	if(fog)
		id |= FOG;
	if(specular_map)
		id |= SPECULAR_MAP;
	if(normal_map)
		id |= NORMAL_MAP;
	if(point_light)
		id |= POINT_LIGHT;
	if(dir_light)
		id |= DIR_LIGHT;
	return id;
}

//=================================================================================================
SuperShader::Shader& SuperShader::GetShader(uint id)
{
	for(Shader& shader : shaders)
	{
		if(shader.id == id)
			return shader;
	}

	// not found, compile
	return CompileShader(id);
}

//=================================================================================================
SuperShader::Shader& SuperShader::CompileShader(uint id)
{
	Info("Compiling super shader %u.", id);

	// setup layout
	VertexDeclarationId vertDecl;
	if(IsSet(id, HAVE_WEIGHT))
	{
		if(IsSet(id, HAVE_TANGENTS))
			vertDecl = VDI_ANIMATED_TANGENT;
		else
			vertDecl = VDI_ANIMATED;
	}
	else
	{
		if(IsSet(id, HAVE_TANGENTS))
			vertDecl = VDI_TANGENT;
		else
			vertDecl = VDI_DEFAULT;
	}

	// setup macros
	D3D_SHADER_MACRO macros[8] = {};
	uint i = 0;

	if(IsSet(id, HAVE_WEIGHT))
	{
		macros[i].Name = "HAVE_WEIGHT";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, HAVE_TANGENTS))
	{
		macros[i].Name = "HAVE_TANGENTS";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, ANIMATED))
	{
		macros[i].Name = "ANIMATED";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, FOG))
	{
		macros[i].Name = "FOG";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, SPECULAR_MAP))
	{
		macros[i].Name = "SPECULAR_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, NORMAL_MAP))
	{
		macros[i].Name = "NORMAL_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, POINT_LIGHT))
	{
		macros[i].Name = "POINT_LIGHT";
		macros[i].Definition = "1";
		++i;
	}
	else if(IsSet(id, DIR_LIGHT))
	{
		macros[i].Name = "DIR_LIGHT";
		macros[i].Definition = "1";
		++i;
	}

	// compile
	Shader shader;
	shader.id = id;
	app::render->CreateShader("super.hlsl", vertDecl, shader.vertexShader, shader.pixelShader, shader.layout, macros);
	shaders.push_back(shader);
	return shaders.back();
}

//=================================================================================================
void SuperShader::Prepare(Scene* scene, Camera* camera)
{
	assert(scene && camera);
	this->camera = camera;

	// bind shader buffers, samplers
	ID3D11Buffer* vsBuffers[] = { vsGlobals, vsLocals };
	deviceContext->VSSetConstantBuffers(0, 2, vsBuffers);

	ID3D11Buffer* psBuffers[] = { psGlobals, psLocals, psMaterial };
	deviceContext->PSSetConstantBuffers(0, 3, psBuffers);

	ID3D11SamplerState* samplers[] = { samplerDiffuse, samplerNormal, samplerSpecular };
	deviceContext->PSSetSamplers(0, 3, samplers);

	// set vertex shader globals
	{
		ResourceLock lock(vsGlobals);
		lock.Get<VsGlobals>()->cameraPos = camera->from;
	}

	// set pixel shader globals
	{
		ResourceLock lock(psGlobals);
		PsGlobals& psg = *lock.Get<PsGlobals>();
		psg.ambientColor = scene->GetAmbientColor();
		psg.lightColor = scene->GetLightColor();
		psg.lightDir = scene->GetLightDir();
		psg.fogColor = scene->GetFogColor();
		psg.fogParams = scene->GetFogParams();
	}
}

//=================================================================================================
void SuperShader::SetShader(uint id)
{
	Shader& shader = GetShader(id);

	deviceContext->IASetInputLayout(shader.layout);
	deviceContext->VSSetShader(shader.vertexShader, nullptr, 0);
	deviceContext->PSSetShader(shader.pixelShader, nullptr, 0);

	applyBones = IsSet(id, ANIMATED);
	applyLights = IsSet(id, POINT_LIGHT);
	applyNormalMap = IsSet(id, NORMAL_MAP);
	applySpecularMap = IsSet(id, SPECULAR_MAP);
	prevMesh = nullptr;
}

//=================================================================================================
void SuperShader::SetTexture(const TexOverride* texOverride, Mesh* mesh, uint index)
{
	TEX tex;
	if(texOverride && texOverride[index].diffuse)
		tex = texOverride[index].diffuse->tex;
	else
		tex = mesh->subs[index].tex->tex;
	deviceContext->PSSetShaderResources(0, 1, &tex);

	if(applyNormalMap)
	{
		if(texOverride && texOverride[index].normal)
			tex = texOverride[index].normal->tex;
		else if(mesh && mesh->subs[index].tex_normal)
			tex = mesh->subs[index].tex_normal->tex;
		else
			tex = texEmptyNormalMap;
		deviceContext->PSSetShaderResources(1, 1, &tex);
	}

	if(applySpecularMap)
	{
		if(texOverride && texOverride[index].specular)
			tex = texOverride[index].specular->tex;
		else if(mesh && mesh->subs[index].tex_specular)
			tex = mesh->subs[index].tex_specular->tex;
		else
			tex = texEmptySpecularMap;
		deviceContext->PSSetShaderResources(2, 1, &tex);
	}
}

//=================================================================================================
void SuperShader::SetCustomMesh(ID3D11Buffer* vb, ID3D11Buffer* ib, uint vertexSize)
{
	uint stride = vertexSize, offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	deviceContext->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);

	// apply vertex shader constants per material (default values)
	{
		ResourceLock lock(psMaterial);
		PsMaterial& psm = *lock.Get<PsMaterial>();
		psm.specularColor = Vec3::One;
		psm.specularHardness = 10.f;
		psm.specularIntensity = 0.2f;
	}
}

//=================================================================================================
void SuperShader::Draw(SceneNode* node)
{
	assert(node);

	Mesh& mesh = *node->mesh;

	// set vertex/index buffer
	if(&mesh != prevMesh)
	{
		uint stride = mesh.vertex_size, offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
		deviceContext->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R16_UINT, 0);
		prevMesh = &mesh;
	}

	// set vertex shader constants per mesh data
	{
		ResourceLock lock(vsLocals);
		VsLocals& vsl = *lock.Get<VsLocals>();
		if(node->type == SceneNode::NORMAL)
			vsl.matCombined = (node->mat * camera->mat_view_proj).Transpose();
		else
			vsl.matCombined = (node->mat.Inverse() * camera->mat_view_proj).Transpose();
		vsl.matWorld = node->mat.Transpose();
		if(applyBones)
		{
			node->mesh_inst->SetupBones();
			for(uint i = 0; i < node->mesh_inst->mesh->head.n_bones; ++i)
				vsl.matBones[i] = node->mesh_inst->mat_bones[i].Transpose();
		}
	}

	// set pixel shader constants per mesh data
	{
		ResourceLock lock(psLocals);
		PsLocals& psl = *lock.Get<PsLocals>();
		psl.tint = node->tint;
		if(applyLights)
		{
			for(uint i = 0; i < Lights::COUNT; ++i)
			{
				if(node->lights[i])
					psl.lights.ld[i] = *node->lights[i];
				else
					psl.lights.ld[i] = Light::EMPTY;
			}
		}
	}

	// for each submesh
	if(!IsSet(node->subs, SceneNode::SPLIT_INDEX))
	{
		for(int i = 0; i < mesh.head.n_subs; ++i)
		{
			if(IsSet(node->subs, 1 << i))
				DrawSubmesh(node, i);
		}
	}
	else
	{
		int index = (node->subs & ~SceneNode::SPLIT_INDEX);
		DrawSubmesh(node, index);
	}
}

//=================================================================================================
void SuperShader::DrawSubmesh(SceneNode* node, uint index)
{
	Mesh::Submesh& sub = node->mesh->subs[index];

	// apply vertex shader constants per material
	{
		ResourceLock lock(psMaterial);
		PsMaterial& psm = *lock.Get<PsMaterial>();
		psm.specularColor = sub.specular_color;
		psm.specularHardness = (float)sub.specular_hardness;
		psm.specularIntensity = sub.specular_intensity;
	}

	// set texture OwO
	SetTexture(node->tex_override, node->mesh, index);

	// actual drawing
	deviceContext->DrawIndexed(sub.tris * 3, sub.first * 3, 0);
}

//=================================================================================================
void SuperShader::DrawCustom(const Matrix& matWorld, const Matrix& matCombined, const std::array<Light*, 3>& lights, uint startIndex, uint indexCount)
{
	// set vertex shader constants per mesh data
	{
		ResourceLock lock(vsLocals);
		VsLocals& vsl = *lock.Get<VsLocals>();
		vsl.matCombined = matCombined.Transpose();
		vsl.matWorld = matWorld.Transpose();
	}

	// set pixel shader constants per mesh data
	{
		ResourceLock lock(psLocals);
		PsLocals& psl = *lock.Get<PsLocals>();
		psl.tint = Vec4::One;
		if(applyLights)
		{
			for(uint i = 0; i < Lights::COUNT; ++i)
			{
				if(lights[i])
					psl.lights.ld[i] = *lights[i];
				else
					psl.lights.ld[i] = Light::EMPTY;
			}
		}
	}

	// actual drawing
	deviceContext->DrawIndexed(indexCount, startIndex, 0);
}
