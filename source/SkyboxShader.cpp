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
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	app::render->CreateShader("skybox.hlsl", desc, countof(desc), vertexShader, pixelShader, layout);
	vsGlobals = app::render->CreateConstantBuffer<VsGlobals>();
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

	app::render->SetAlphaTest(false);
	app::render->SetAlphaBlend(false);
	app::render->SetNoCulling(false);
	app::render->SetNoZWrite(true);

	// setup shader
	deviceContext->IASetInputLayout(layout);
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, &sampler);
	uint stride = sizeof(VDefault),
		offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &mesh.vb, &stride, &offset);
	deviceContext->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R16_UINT, 0);

	// vertex shader constants
	D3D11_MAPPED_SUBRESOURCE resource;
	V(deviceContext->Map(vsGlobals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VsGlobals& vsg = *(VsGlobals*)resource.pData;
	vsg.matCombined = (Matrix::Translation(camera.from) * camera.mat_view_proj).Transpose();
	deviceContext->Unmap(vsGlobals, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);

	// draw
	for(Mesh::Submesh& sub : mesh.subs)
	{
		deviceContext->PSSetShaderResources(0, 1, &sub.tex->tex);
		deviceContext->DrawIndexed(sub.tris * 3, sub.first * 3, sub.min_ind);
	}
}
