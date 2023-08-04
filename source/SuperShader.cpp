#include "Pch.h"
#include "SuperShader.h"

#include "Camera.h"
#include "DirectX.h"
#include "File.h"
#include "Light.h"
#include "Mesh.h"
#include "Render.h"
#include "Scene.h"
#include "SceneManager.h"
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
	float alphaTest;
};

struct PsMaterial
{
	Vec3 specularColor;
	float specularHardness;
	float specularIntensity;
};

//=================================================================================================
SuperShader::SuperShader() : deviceContext(app::render->GetDeviceContext()), vsGlobals(nullptr), vsLocals(nullptr), psGlobals(nullptr), psLocals(nullptr),
psMaterial(nullptr), texEmptyNormalMap(nullptr), texEmptySpecularMap(nullptr), vbDecal(nullptr), ibDecal(nullptr)
{
}

//=================================================================================================
void SuperShader::OnInit()
{
	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "SuperVsGlobals");
	vsLocals = app::render->CreateConstantBuffer(sizeof(VsLocals), "SuperVsLocals");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "SuperPsGlobals");
	psLocals = app::render->CreateConstantBuffer(sizeof(PsLocals), "SuperPsLocals");
	psMaterial = app::render->CreateConstantBuffer(sizeof(PsMaterial), "SuperPsMaterial");

	const Color emptyNormalMapColor = Color(128, 128, 255);
	texEmptyNormalMap = app::render->CreateImmutableTexture(Int2(1, 1), &emptyNormalMapColor);
	texEmptySpecularMap = app::render->CreateImmutableTexture(Int2(1, 1), &Color::None);

	// load shader code
	cstring path = Format("%s/super.hlsl", app::render->GetShadersDir().c_str());
	if(!io::LoadFileToString(path, code))
		throw Format("Failed to load '%s'.", path);

	// create decal vertex buffer
	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(VDefault) * 4;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	V(app::render->GetDevice()->CreateBuffer(&desc, nullptr, &vbDecal));
	SetDebugName(vbDecal, "DecalVb");

	// create decal index buffer
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = sizeof(word) * 6;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;

	word ids[] = { 0, 1, 2, 2, 1, 3 };
	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = ids;

	V(app::render->GetDevice()->CreateBuffer(&desc, &data, &ibDecal));
	SetDebugName(ibDecal, "DecalIb");
}

//=================================================================================================
void SuperShader::OnRelease()
{
	for(auto& it : shaders)
	{
		Shader& shader = it.second;
		SafeRelease(shader.pixelShader);
		SafeRelease(shader.vertexShader);
		SafeRelease(shader.layout);
	}
	shaders.clear();

	SafeRelease(vsGlobals);
	SafeRelease(vsLocals);
	SafeRelease(psGlobals);
	SafeRelease(psLocals);
	SafeRelease(psMaterial);
	SafeRelease(texEmptyNormalMap);
	SafeRelease(texEmptySpecularMap);
	SafeRelease(vbDecal);
	SafeRelease(ibDecal);
}

//=================================================================================================
uint SuperShader::GetShaderId(bool haveWeights, bool haveTangents, bool animated, bool fog, bool specularMap,
	bool normalMap, bool pointLight, bool dirLight) const
{
	uint id = 0;
	if(haveWeights)
		id |= HAVE_WEIGHT;
	if(haveTangents)
		id |= HAVE_TANGENTS;
	if(animated)
		id |= ANIMATED;
	if(fog)
		id |= FOG;
	if(specularMap)
		id |= SPECULAR_MAP;
	if(normalMap)
		id |= NORMAL_MAP;
	if(pointLight)
		id |= POINT_LIGHT;
	if(dirLight)
		id |= DIR_LIGHT;
	return id;
}

//=================================================================================================
SuperShader::Shader& SuperShader::GetShader(uint id)
{
	auto it = shaders.find(id);
	if(it != shaders.end())
		return it->second;

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

	Render::ShaderParams params;
	params.name = "super";
	params.cacheName = Format("super%u", id);
	params.code = &code;
	params.decl = vertDecl;
	params.vertexShader = &shader.vertexShader;
	params.pixelShader = &shader.pixelShader;
	params.layout = &shader.layout;
	params.macro = macros;
	app::render->CreateShader(params);

	shaders[id] = shader;
	return shaders[id];
}

//=================================================================================================
void SuperShader::SetScene(Scene* scene, Camera* camera)
{
	assert(scene && camera);
	this->scene = scene;
	this->camera = camera;

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
void SuperShader::Prepare()
{
	// bind shader buffers, samplers
	ID3D11Buffer* vsBuffers[] = { vsGlobals, vsLocals };
	deviceContext->VSSetConstantBuffers(0, 2, vsBuffers);

	ID3D11Buffer* psBuffers[] = { psGlobals, psLocals, psMaterial };
	deviceContext->PSSetConstantBuffers(0, 3, psBuffers);

	ID3D11SamplerState* sampler = app::render->GetSampler();
	deviceContext->PSSetSamplers(0, 1, &sampler);
}

//=================================================================================================
void SuperShader::PrepareDecals()
{
	Prepare();
	SetCustomMesh(vbDecal, ibDecal, sizeof(VDefault));

	app::render->SetBlendState(Render::BLEND_ADD);
	app::render->SetDepthState(Render::DEPTH_READ);
	app::render->SetRasterState(Render::RASTER_NORMAL);

	const bool useFog = app::sceneMgr->useLighting && app::sceneMgr->useFog;

	SetShader(GetShaderId(false, false, false, useFog, false, false,
		!scene->useLightDir && app::sceneMgr->useLighting, scene->useLightDir && app::sceneMgr->useLighting));
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
void SuperShader::SetTexture(Texture* tex)
{
	assert(tex);
	TEX t = tex->tex;
	deviceContext->PSSetShaderResources(0, 1, &t);
}

//=================================================================================================
void SuperShader::SetTexture(const TexOverride* texOverride, Mesh* mesh, uint index)
{
	TEX tex;
	if(texOverride && texOverride[index].diffuse)
		tex = texOverride[index].diffuse->tex;
	else
		tex = mesh->subs[index].tex->tex;
	assert(tex);
	deviceContext->PSSetShaderResources(0, 1, &tex);

	if(applyNormalMap)
	{
		if(texOverride && texOverride[index].normal)
			tex = texOverride[index].normal->tex;
		else if(mesh && mesh->subs[index].texNormal)
			tex = mesh->subs[index].texNormal->tex;
		else
			tex = texEmptyNormalMap;
		deviceContext->PSSetShaderResources(1, 1, &tex);
	}

	if(applySpecularMap)
	{
		if(texOverride && texOverride[index].specular)
			tex = texOverride[index].specular->tex;
		else if(mesh && mesh->subs[index].texSpecular)
			tex = mesh->subs[index].texSpecular->tex;
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
		uint stride = mesh.vertexSize, offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
		deviceContext->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R16_UINT, 0);
		prevMesh = &mesh;
	}

	// set vertex shader constants per mesh data
	{
		ResourceLock lock(vsLocals);
		VsLocals& vsl = *lock.Get<VsLocals>();
		vsl.matCombined = (node->mat * camera->matViewProj).Transpose();
		vsl.matWorld = node->mat.Transpose();
		if(applyBones)
		{
			const vector<Matrix>& matBones = node->meshInst->GetBoneMatrices();
			for(uint i = 0, count = matBones.size(); i < count; ++i)
				vsl.matBones[i] = matBones[i].Transpose();
		}
	}

	// set pixel shader constants per mesh data
	{
		ResourceLock lock(psLocals);
		PsLocals& psl = *lock.Get<PsLocals>();
		psl.tint = node->tint;
		psl.alphaTest = 0.5f;
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
		for(int i = 0; i < mesh.head.nSubs; ++i)
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
		psm.specularColor = sub.specularColor;
		psm.specularHardness = (float)sub.specularHardness;
		psm.specularIntensity = sub.specularIntensity;
	}

	// set texture OwO
	SetTexture(node->texOverride, node->mesh, index);

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
		psl.alphaTest = 0.5f;
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

//=================================================================================================
void SuperShader::DrawDecal(const Decal& decal)
{
	// set decal vertices
	{
		ResourceLock lock(vbDecal);
		VDefault* v = lock.Get<VDefault>();
		v[0].tex = Vec2(0, 0);
		v[1].tex = Vec2(0, 1);
		v[2].tex = Vec2(1, 0);
		v[3].tex = Vec2(1, 1);
		for(int i = 0; i < 4; ++i)
		{
			v[i].pos.y = 0;
			v[i].normal = decal.normal;
		}
		if(decal.normal.Equal(Vec3(0, 1, 0)))
		{
			v[0].pos.x = decal.scale * sin(decal.rot + 5.f / 4 * PI);
			v[0].pos.z = decal.scale * cos(decal.rot + 5.f / 4 * PI);
			v[1].pos.x = decal.scale * sin(decal.rot + 7.f / 4 * PI);
			v[1].pos.z = decal.scale * cos(decal.rot + 7.f / 4 * PI);
			v[2].pos.x = decal.scale * sin(decal.rot + 3.f / 4 * PI);
			v[2].pos.z = decal.scale * cos(decal.rot + 3.f / 4 * PI);
			v[3].pos.x = decal.scale * sin(decal.rot + 1.f / 4 * PI);
			v[3].pos.z = decal.scale * cos(decal.rot + 1.f / 4 * PI);
		}
		else
		{
			const Vec3 front(sin(decal.rot), 0, cos(decal.rot)), right(sin(decal.rot + PI / 2), 0, cos(decal.rot + PI / 2));
			Vec3 vX, vZ, vLx, vRx, vLz, vRz;
			vX = decal.normal.Cross(front);
			vZ = decal.normal.Cross(right);
			if(vX.x > 0.f)
			{
				vRx = vX * decal.scale;
				vLx = -vX * decal.scale;
			}
			else
			{
				vRx = -vX * decal.scale;
				vLx = vX * decal.scale;
			}
			if(vZ.z > 0.f)
			{
				vRz = vZ * decal.scale;
				vLz = -vZ * decal.scale;
			}
			else
			{
				vRz = -vZ * decal.scale;
				vLz = vZ * decal.scale;
			}

			v[0].pos = vLx + vLz;
			v[1].pos = vLx + vRz;
			v[2].pos = vRx + vLz;
			v[3].pos = vRx + vRz;
		}
	}

	// set vertex shader constants
	{
		ResourceLock lock(vsLocals);
		VsLocals& vsl = *lock.Get<VsLocals>();
		Matrix matWorld = Matrix::Translation(decal.pos);
		vsl.matWorld = matWorld.Transpose();
		vsl.matCombined = (matWorld * camera->matViewProj).Transpose();
	}

	// set pixel shader constants
	{
		ResourceLock lock(psLocals);
		PsLocals& psl = *lock.Get<PsLocals>();
		psl.tint = Vec4::One;
		psl.alphaTest = 0;
		if(applyLights)
		{
			const std::array<Light*, 3>& lights = *decal.lights;
			for(uint i = 0; i < Lights::COUNT; ++i)
			{
				if(lights[i])
					psl.lights.ld[i] = *lights[i];
				else
					psl.lights.ld[i] = Light::EMPTY;
			}
		}
	}

	deviceContext->PSSetShaderResources(0, 1, &decal.tex);
	deviceContext->DrawIndexed(6, 0, 0);
}
