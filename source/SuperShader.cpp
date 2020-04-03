#include "Pch.h"
#include "SuperShader.h"

//#include "File.h"
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
vsGlobals(nullptr), vsLocals(nullptr), psGlobals(nullptr), psLocals(nullptr), psMaterial(nullptr)
{
}

//=================================================================================================
void SuperShader::OnInit()
{
	/*if(!pool)
	{
		V(D3DXCreateEffectPool(&pool));

		tex_empty_normal_map = app::render->CreateTexture(Int2(1, 1), &Color(128, 128, 255));
		tex_empty_specular_map = app::render->CreateTexture(Int2(1, 1), &Color::None);
	}

	cstring path = Format("%s/super.fx", app::render->GetShadersDir().c_str());
	FileReader f(path);
	if(!f)
		throw Format("Failed to open file '%s'.", path);
	FileTime file_time = f.GetTime();
	if(file_time != edit_time)
	{
		f.ReadToString(code);
		edit_time = file_time;
	}

	Info("Setting up super shader parameters.");
	GetShader(0);
	ID3DXEffect* e = shaders[0].e;
	hMatCombined = e->GetParameterByName(nullptr, "matCombined");
	hMatWorld = e->GetParameterByName(nullptr, "matWorld");
	hMatBones = e->GetParameterByName(nullptr, "matBones");
	hTint = e->GetParameterByName(nullptr, "tint");
	hAmbientColor = e->GetParameterByName(nullptr, "ambientColor");
	hFogColor = e->GetParameterByName(nullptr, "fogColor");
	hFogParams = e->GetParameterByName(nullptr, "fogParams");
	hLightDir = e->GetParameterByName(nullptr, "lightDir");
	hLightColor = e->GetParameterByName(nullptr, "lightColor");
	hLights = e->GetParameterByName(nullptr, "lights");
	hSpecularColor = e->GetParameterByName(nullptr, "specularColor");
	hSpecularIntensity = e->GetParameterByName(nullptr, "specularIntensity");
	hSpecularHardness = e->GetParameterByName(nullptr, "specularHardness");
	hCameraPos = e->GetParameterByName(nullptr, "cameraPos");
	hTexDiffuse = e->GetParameterByName(nullptr, "texDiffuse");
	hTexNormal = e->GetParameterByName(nullptr, "texNormal");
	hTexSpecular = e->GetParameterByName(nullptr, "texSpecular");
	assert(hMatCombined && hMatWorld && hMatBones && hTint && hAmbientColor && hFogColor && hFogParams && hLightDir && hLightColor && hLights && hSpecularColor
		&& hSpecularIntensity && hSpecularHardness && hCameraPos && hTexDiffuse && hTexNormal && hTexSpecular);*/

	samplerDiffuse = app::render->CreateSampler();
	samplerNormal = app::render->CreateSampler();
	samplerSpecular = app::render->CreateSampler();

	vsGlobals = app::render->CreateConstantBuffer<VsGlobals>();
	vsLocals = app::render->CreateConstantBuffer<VsLocals>();
	psGlobals = app::render->CreateConstantBuffer<PsGlobals>();
	psLocals = app::render->CreateConstantBuffer<PsLocals>();
	psMaterial = app::render->CreateConstantBuffer<PsMaterial>();
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
/*void SuperShader::ApplyLights(const array<Light*, 3>& lights)
{
	Lights l;
	for(uint i = 0; i < 3; ++i)
	{
		if(lights[i])
		{
			l.ld[i].pos = lights[i]->pos;
			l.ld[i].range = lights[i]->range;
			l.ld[i].color = lights[i]->color;
		}
		else
		{
			l.ld[i].pos = Vec3::Zero;
			l.ld[i].range = 1;
			l.ld[i].color = Vec4::Zero;
		}
	}
	V(GetEffect()->SetRawValue(hLights, &l, 0, sizeof(Lights)));
}*/

SuperShader::Shader& SuperShader::CompileShader(uint id)
{
	Info("Compiling super shader %u.", id);

	// setup layout
	vector<D3D11_INPUT_ELEMENT_DESC> layoutDesc;
	layoutDesc.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	if(IsSet(id, HAVE_WEIGHT))
	{
		layoutDesc.push_back({ "BLENDWEIGHT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		layoutDesc.push_back({ "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	}
	layoutDesc.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	layoutDesc.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	if(IsSet(id, HAVE_TANGENTS))
	{
		layoutDesc.push_back({ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		layoutDesc.push_back({ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
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
	app::render->CreateShader("super.hlsl", layoutDesc.data(), layoutDesc.size(), shader.vertexShader, shader.pixelShader, shader.layout, macros);
	shaders.push_back(shader);
	return shaders.back();
}

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
	D3D11_MAPPED_SUBRESOURCE resource;
	V(deviceContext->Map(vsGlobals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VsGlobals& vsg = *(VsGlobals*)resource.pData;
	vsg.cameraPos = camera->from;
	deviceContext->Unmap(vsGlobals, 0);

	// set pixel shader globals
	V(deviceContext->Map(psGlobals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PsGlobals& psg = *(PsGlobals*)resource.pData;
	psg.ambientColor = scene->GetAmbientColor();
	psg.lightColor = scene->GetLightColor();
	psg.lightDir = scene->GetLightDir();
	psg.fogColor = scene->GetFogColor();
	psg.fogParams = scene->GetFogParams();
	deviceContext->Unmap(psGlobals, 0);
}

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
	D3D11_MAPPED_SUBRESOURCE resource;
	V(deviceContext->Map(vsLocals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VsLocals& vsl = *(VsLocals*)resource.pData;
	vsl.matCombined = (node->mat * camera->mat_view_proj).Transpose();
	vsl.matWorld = node->mat.Transpose();
	if(applyBones)
	{
		node->mesh_inst->SetupBones();
		for(uint i = 0; i < mesh.head.n_bones; ++i)
			vsl.matBones[i] = node->mesh_inst->mat_bones[i].Transpose();
	}
	deviceContext->Unmap(vsLocals, 0);

	// set pixel shader constants per mesh data
	V(deviceContext->Map(psLocals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PsLocals& psl = *(PsLocals*)resource.pData;
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
	deviceContext->Unmap(psLocals, 0);

	// for each submesh
	if(!IsSet(node->subs, SceneNode::SPLIT_INDEX))
	{
		for(int i = 0; i < mesh.head.n_subs; ++i)
		{
			if(IsSet(node->subs, 1 << i))
				DrawSubmesh(mesh.subs[i]);
		}
	}
	else
	{
		int index = (node->subs & ~SceneNode::SPLIT_INDEX);
		DrawSubmesh(mesh.subs[index]);
	}
}

void SuperShader::DrawSubmesh(Mesh::Submesh& sub)
{
	// apply vertex shader constants per material
	D3D11_MAPPED_SUBRESOURCE resource;
	V(deviceContext->Map(psMaterial, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PsMaterial& psm = *(PsMaterial*)resource.pData;
	psm.specularColor = sub.specular_color;
	psm.specularHardness = (float)sub.specular_hardness;
	psm.specularIntensity = sub.specular_intensity;
	deviceContext->Unmap(psMaterial, 0);

	// apply textures
	deviceContext->PSSetShaderResources(0, 1, &sub.tex->tex);
	if(applyNormalMap)
		deviceContext->PSSetShaderResources(1, 1, sub.tex_normal ? &sub.tex_normal->tex : &texEmptyNormalMap);
	if(applySpecularMap)
		deviceContext->PSSetShaderResources(2, 1, sub.tex_specular ? &sub.tex_specular->tex : &texEmptySpecularMap);

	// actual drawing
	deviceContext->DrawIndexed(sub.tris * 3, sub.first * 3, sub.min_ind);
}
