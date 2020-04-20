#include "Pch.h"
#include "SkyboxShader.h"

#include "Camera.h"
#include "DirectX.h"
#include "Mesh.h"
#include "Render.h"

struct VsGlobals
{
	Matrix matCombined;
};

//=================================================================================================
SkyboxShader::SkyboxShader() : deviceContext(app::render->GetDeviceContext()), vertexShader(nullptr), pixelShader(nullptr), layout(nullptr),
vsGlobals(nullptr), sampler(nullptr)
{
}

//=================================================================================================
void SkyboxShader::OnInit()
{
	app::render->CreateShader("skybox.hlsl", VDI_DEFAULT, vertexShader, pixelShader, layout);
	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "SkyboxVsGlobals");
	sampler = app::render->CreateSampler(Render::TEX_ADR_CLAMP);
}

//=================================================================================================
void SkyboxShader::OnRelease()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShader);
	SafeRelease(layout);
	SafeRelease(vsGlobals);
	SafeRelease(sampler);
}

//=================================================================================================
void SkyboxShader::Draw(Mesh& mesh, Camera& camera)
{
	assert(mesh.vertex_decl == VDI_DEFAULT);

	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_NO);
	app::render->SetRasterState(Render::RASTER_NORMAL);

	// setup shader
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, &sampler);
	uint stride = sizeof(VDefault),
		offset = 0;
	deviceContext->IASetInputLayout(layout);
	deviceContext->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
	deviceContext->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R16_UINT, 0);

	// vertex shader constants
	{
		ResourceLock lock(vsGlobals);
		lock.Get<VsGlobals>()->matCombined = (Matrix::Translation(camera.from) * camera.mat_view_proj).Transpose();
	}

	// draw
	for(Mesh::Submesh& sub : mesh.subs)
	{
		deviceContext->PSSetShaderResources(0, 1, &sub.tex->tex);
		deviceContext->DrawIndexed(sub.tris * 3, sub.first * 3, 0);
	}
}
