#include "Pch.h"
#include "TerrainShader.h"
#include "Terrain.h"
#include "Camera.h"
#include "Texture.h"
#include "Render.h"
#include "SceneManager.h"
#include "Scene.h"
#include "DirectX.h"

struct VsGlobals
{
	Matrix matCombined;
	Matrix matWorld;
};

struct PsGlobals
{
	Vec4 colorAmbient;
	Vec4 colorDiffuse;
	Vec3 lightDir;
	float _pad;
	Vec4 fogColor;
	Vec4 fogParam;
};

//=================================================================================================
TerrainShader::TerrainShader() : device_context(app::render->GetDeviceContext()), vertex_shader(nullptr), pixel_shader(nullptr), layout(nullptr),
vs_globals(nullptr), ps_globals(nullptr), samplers()
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
	app::render->CreateShader("terrain.hlsl", desc, countof(desc), vertex_shader, pixel_shader, layout);
	vs_globals = app::render->CreateConstantBuffer(sizeof(VsGlobals));
	ps_globals = app::render->CreateConstantBuffer(sizeof(PsGlobals));
	samplers[0] = app::render->CreateSampler(Render::TEX_ADR_CLAMP, Render::FILTER_NONE);
	samplers[1] = app::render->CreateSampler();
	samplers[2] = app::render->CreateSampler();
	samplers[3] = app::render->CreateSampler();
	samplers[4] = app::render->CreateSampler();
	samplers[5] = app::render->CreateSampler();
}

//=================================================================================================
void TerrainShader::OnRelease()
{
	SafeRelease(vertex_shader);
	SafeRelease(pixel_shader);
	SafeRelease(layout);
	SafeRelease(vs_globals);
	SafeRelease(ps_globals);
	for(int i = 0; i < 6; ++i)
		SafeRelease(samplers[i]);
}

//=================================================================================================
void TerrainShader::Draw(Scene* scene, Camera* camera, Terrain* terrain, const vector<uint>& parts)
{
	assert(scene && camera && terrain);

	app::render->SetAlphaTest(false);
	app::render->SetAlphaBlend(false);
	app::render->SetNoCulling(false);
	app::render->SetNoZWrite(false);

	// setup shader
	device_context->IASetInputLayout(layout);
	device_context->VSSetShader(vertex_shader, nullptr, 0);
	device_context->PSSetShader(pixel_shader, nullptr, 0);
	device_context->PSSetSamplers(0, 6, samplers);
	uint stride = sizeof(TerrainVertex),
		offset = 0;
	//device_context->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
	//device_context->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R16_UINT, 0);
	FIXME; // vb, ib ?

	// vertex shader constants
	D3D11_MAPPED_SUBRESOURCE resource;
	V(device_context->Map(vs_globals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VsGlobals& vsg = *(VsGlobals*)resource.pData;
	vsg.matCombined = camera->mat_view_proj.Transpose();
	vsg.matWorld = Matrix::IdentityMatrix.Transpose();
	device_context->Unmap(vs_globals, 0);
	device_context->VSSetConstantBuffers(0, 1, &vs_globals);

	// pixel shader constants
	V(device_context->Map(ps_globals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PsGlobals& psg = *(PsGlobals*)resource.pData;
	psg.colorAmbient = scene->GetAmbientColor();
	psg.colorDiffuse = scene->GetLightColor();
	psg.lightDir = scene->GetLightDir();
	psg.fogColor = scene->GetFogColor();
	psg.fogParam = scene->GetFogParams();
	device_context->Unmap(vs_globals, 0);
	device_context->PSSetConstantBuffers(0, 1, &ps_globals);

	// set textures
	ID3D11ShaderResourceView* textures[6];
	textures[0] = terrain->GetSplatTexture();
	TexturePtr* tex = terrain->GetTextures();
	for(int i = 0; i < 5; ++i)
		textures[i + 1] = tex[i];
	device_context->PSSetShaderResources(0, 6, textures);

	// draw
	uint n_verts, part_tris;
	terrain->GetDrawOptions(n_verts, part_tris);
	for(uint part : parts)
		device_context->DrawIndexed(part_tris * 3, part_tris * part * 3, 0);
}
