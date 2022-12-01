#include "Pch.h"
#include "TerrainShader.h"

#include "Camera.h"
#include "DirectX.h"
#include "Render.h"
#include "Scene.h"
#include "Terrain.h"
#include "Texture.h"

struct VsGlobals
{
	Matrix matCombined;
	Matrix matWorld;
};

struct PsGlobals
{
	Vec4 colorAmbient;
	Vec4 colorDiffuse;
	Vec4 lightDir;
	Vec4 fogColor;
	Vec4 fogParam;
};

//=================================================================================================
TerrainShader::TerrainShader() : deviceContext(app::render->GetDeviceContext()), vertexShader(nullptr), pixelShader(nullptr), layout(nullptr),
vsGlobals(nullptr), psGlobals(nullptr), sampler(nullptr)
{
}

//=================================================================================================
void TerrainShader::OnInit()
{
	Render::ShaderParams params;
	params.name = "terrain";
	params.decl = VDI_TERRAIN;
	params.vertexShader = &vertexShader;
	params.pixelShader = &pixelShader;
	params.layout = &layout;
	app::render->CreateShader(params);

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "TerrainVsGlobals");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "TerrainPsGlobals");
	sampler = app::render->CreateSampler(Render::TEX_ADR_CLAMP);
}

//=================================================================================================
void TerrainShader::OnRelease()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShader);
	SafeRelease(layout);
	SafeRelease(vsGlobals);
	SafeRelease(psGlobals);
	SafeRelease(sampler);
}

//=================================================================================================
void TerrainShader::Draw(Scene* scene, Camera* camera, Terrain* terrain, const vector<uint>& parts)
{
	assert(scene && camera && terrain);

	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_YES);
	app::render->SetRasterState(Render::RASTER_NORMAL);

	// setup shader
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	ID3D11SamplerState* samplers[] = { sampler, app::render->GetSampler() };
	deviceContext->PSSetSamplers(0, 2, samplers);
	uint stride = sizeof(VTerrain), offset = 0;
	deviceContext->IASetInputLayout(layout);
	deviceContext->IASetVertexBuffers(0, 1, &terrain->vb, &stride, &offset);
	deviceContext->IASetIndexBuffer(terrain->ib, DXGI_FORMAT_R32_UINT, 0);

	// vertex shader constants
	{
		ResourceLock lock(vsGlobals);
		VsGlobals& vsg = *lock.Get<VsGlobals>();
		vsg.matCombined = camera->matViewProj.Transpose();
		vsg.matWorld = Matrix::IdentityMatrix.Transpose();
	}

	// pixel shader constants
	{
		ResourceLock lock(psGlobals);
		PsGlobals& psg = *lock.Get<PsGlobals>();
		psg.colorAmbient = scene->GetAmbientColor();
		psg.colorDiffuse = scene->GetLightColor();
		psg.lightDir = scene->GetLightDir();
		psg.fogColor = scene->GetFogColor();
		psg.fogParam = scene->GetFogParams();
	}

	// set textures
	ID3D11ShaderResourceView* textures[6];
	textures[0] = terrain->GetSplatTexture().tex;
	TexturePtr* tex = terrain->GetTextures();
	for(int i = 0; i < 5; ++i)
		textures[i + 1] = tex[i]->tex;
	deviceContext->PSSetShaderResources(0, 6, textures);

	// draw
	for(uint part : parts)
		deviceContext->DrawIndexed(terrain->partTris * 3, terrain->partTris * part * 3, 0);
}
