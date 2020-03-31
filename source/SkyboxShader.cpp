#include "Pch.h"
#include "SkyboxShader.h"
#include "Render.h"
#include "Mesh.h"
#include "Camera.h"
#include "DirectX.h"

//=================================================================================================
SkyboxShader::SkyboxShader() : device_context(app::render->GetDeviceContext()), vertex_shader(nullptr), pixel_shader(nullptr), layout(nullptr),
vs_globals(nullptr), sampler(nullptr)
{
}

//=================================================================================================
void SkyboxShader::OnInit()
{
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	app::render->CreateShader("skybox.hlsl", desc, countof(desc), vertex_shader, pixel_shader, layout);
	vs_globals = app::render->CreateConstantBuffer(sizeof(Matrix));
	sampler = app::render->CreateSampler(Render::TEX_ADR_CLAMP);
}

//=================================================================================================
void SkyboxShader::OnRelease()
{
	SafeRelease(vertex_shader);
	SafeRelease(pixel_shader);
	SafeRelease(layout);
	SafeRelease(vs_globals);
	SafeRelease(sampler);
}

//=================================================================================================
void SkyboxShader::Draw(Mesh& mesh, Camera& camera)
{
	assert(mesh.vertex_decl == VDI_DEFAULT);

	app::render->SetAlphaTest(false);
	app::render->SetAlphaBlend(false);
	app::render->SetNoCulling(false);
	app::render->SetNoZWrite(true);

	// setup shader
	device_context->IASetInputLayout(layout);
	device_context->VSSetShader(vertex_shader, nullptr, 0);
	device_context->PSSetShader(pixel_shader, nullptr, 0);
	device_context->PSSetSamplers(0, 1, &sampler);
	uint stride = sizeof(VDefault),
		offset = 0;
	device_context->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
	device_context->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R16_UINT, 0);

	// vertex shader constants
	Matrix mat_combined = (Matrix::Translation(camera.from) * camera.mat_view_proj).Transpose();
	D3D11_MAPPED_SUBRESOURCE resource;
	V(device_context->Map(vs_globals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	*(Matrix*)resource.pData = mat_combined;
	device_context->Unmap(vs_globals, 0);
	device_context->VSSetConstantBuffers(0, 1, &vs_globals);

	// draw
	for(Mesh::Submesh& sub : mesh.subs)
	{
		device_context->PSSetShaderResources(0, 1, &sub.tex->tex);
		device_context->DrawIndexed(sub.tris * 3, sub.first * 3, sub.min_ind);
	}
}
