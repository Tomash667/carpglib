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
vsGlobals(nullptr), psGlobals(nullptr), samplers()
{
}

//=================================================================================================
void TerrainShader::OnInit()
{
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	app::render->CreateShader("terrain.hlsl", VDI_TERRAIN, vertexShader, pixelShader, layout);
	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "TerrainVsGlobals");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "TerrainPsGlobals");
	samplers[0] = app::render->CreateSampler(Render::TEX_ADR_CLAMP);
	samplers[1] = app::render->CreateSampler();
	samplers[2] = app::render->CreateSampler();
	samplers[3] = app::render->CreateSampler();
	samplers[4] = app::render->CreateSampler();
	samplers[5] = app::render->CreateSampler();
}

//=================================================================================================
void TerrainShader::OnRelease()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShader);
	SafeRelease(layout);
	SafeRelease(vsGlobals);
	SafeRelease(psGlobals);
	for(int i = 0; i < 6; ++i)
		SafeRelease(samplers[i]);
}

//=================================================================================================
void TerrainShader::Draw(Scene* scene, Camera* camera, Terrain* terrain, const vector<uint>& parts)
{
	assert(scene && camera && terrain);

	app::render->SetAlphaBlend(false);
	app::render->SetDepthState(Render::DEPTH_YES);
	app::render->SetNoCulling(false);

	// setup shader
	deviceContext->IASetInputLayout(layout);
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetSamplers(0, 6, samplers);
	uint stride = sizeof(TerrainVertex),
		offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &terrain->vb, &stride, &offset);
	deviceContext->IASetIndexBuffer(terrain->ib, DXGI_FORMAT_R32_UINT, 0);

	// vertex shader constants
	D3D11_MAPPED_SUBRESOURCE resource;
	V(deviceContext->Map(vsGlobals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VsGlobals& vsg = *(VsGlobals*)resource.pData;
	vsg.matCombined = camera->mat_view_proj.Transpose();
	vsg.matWorld = Matrix::IdentityMatrix.Transpose();
	deviceContext->Unmap(vsGlobals, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);

	// pixel shader constants
	V(deviceContext->Map(psGlobals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PsGlobals& psg = *(PsGlobals*)resource.pData;
	psg.colorAmbient = scene->GetAmbientColor();
	psg.colorDiffuse = scene->GetLightColor();
	psg.lightDir = scene->GetLightDir();
	psg.fogColor = scene->GetFogColor();
	psg.fogParam = scene->GetFogParams();
	deviceContext->Unmap(psGlobals, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);

	// set textures
	ID3D11ShaderResourceView* textures[6];
	textures[0] = terrain->GetSplatTexture();
	TexturePtr* tex = terrain->GetTextures();
	for(int i = 0; i < 5; ++i)
		textures[i + 1] = tex[i]->tex;
	deviceContext->PSSetShaderResources(0, 6, textures);

	// draw
	for(uint part : parts)
		deviceContext->DrawIndexed(terrain->part_tris * 3, terrain->part_tris * part * 3, 0);
	FIXME; // verify args
}
