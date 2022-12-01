#include "Pch.h"
#include "GrassShader.h"

#include "Camera.h"
#include "DirectX.h"
#include "Mesh.h"
#include "Render.h"
#include "Scene.h"
#include "SceneManager.h"

struct VsGlobals
{
	Matrix matViewProj;
};

struct PsGlobals
{
	Vec4 fogColor;
	Vec4 fogParams;
	Vec4 ambientColor;
};

//=================================================================================================
GrassShader::GrassShader() : deviceContext(app::render->GetDeviceContext()), vertexShader(nullptr), pixelShader(nullptr), layout(nullptr),
vsGlobals(nullptr), psGlobals(nullptr), vb(nullptr), vbSize(0)
{
}

//=================================================================================================
void GrassShader::OnInit()
{
	Render::ShaderParams params;
	params.name = "grass";
	params.decl = VDI_GRASS;
	params.vertexShader = &vertexShader;
	params.pixelShader = &pixelShader;
	params.layout = &layout;
	app::render->CreateShader(params);

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals));
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals));
}

//=================================================================================================
void GrassShader::OnRelease()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShader);
	SafeRelease(layout);
	SafeRelease(vsGlobals);
	SafeRelease(psGlobals);
	SafeRelease(vb);
	vbSize = 0;
}

//=================================================================================================
void GrassShader::Prepare(Scene* scene, Camera* camera)
{
	assert(scene && camera);

	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_YES);
	app::render->SetRasterState(Render::RASTER_NO_CULLING);

	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	ID3D11SamplerState* sampler = app::render->GetSampler();
	deviceContext->PSSetSamplers(0, 1, &sampler);
	deviceContext->IASetInputLayout(layout);

	// set matrix
	{
		ResourceLock lock(vsGlobals);
		VsGlobals& vsg = *lock.Get<VsGlobals>();
		vsg.matViewProj = camera->matViewProj.Transpose();
	}

	// set pixel shader globals
	{
		ResourceLock lock(psGlobals);
		PsGlobals& psg = *lock.Get<PsGlobals>();
		psg.ambientColor = scene->GetAmbientColor();
		psg.fogColor = scene->GetFogColor();
		psg.fogParams = scene->GetFogParams();
	}
}

//=================================================================================================
void GrassShader::ReserveVertexBuffer(uint vertexCount)
{
	if(vertexCount <= vbSize)
		return;

	SafeRelease(vb);
	vbSize = vertexCount;

	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(Matrix) * vertexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	V(app::render->GetDevice()->CreateBuffer(&desc, nullptr, &vb));
	SetDebugName(vb, "GrassVb");
}

//=================================================================================================
void GrassShader::Draw(Mesh* mesh, const vector<const vector<Matrix>*>& patches, uint count)
{
	assert(mesh->vertexDecl == VDI_DEFAULT);

	ReserveVertexBuffer(count);

	ID3D11Buffer* vbs[2] = { mesh->vb, vb };
	uint size[2] = { sizeof(VDefault), sizeof(Matrix) };
	uint offset[2] = { 0, 0 };
	deviceContext->IASetInputLayout(layout);
	deviceContext->IASetVertexBuffers(0, 2, vbs, size, offset);
	deviceContext->IASetIndexBuffer(mesh->ib, DXGI_FORMAT_R16_UINT, 0);

	// setup instancing data
	{
		ResourceLock lock(vb);
		Matrix* m = lock.Get<Matrix>();
		int index = 0;
		for(vector< const vector<Matrix>* >::const_iterator it = patches.begin(), end = patches.end(); it != end; ++it)
		{
			const vector<Matrix>& vm = **it;
			memcpy(&m[index], &vm[0], sizeof(Matrix) * vm.size());
			index += vm.size();
		}
	}

	// draw
	for(Mesh::Submesh& sub : mesh->subs)
	{
		deviceContext->PSSetShaderResources(0, 1, &sub.tex->tex);
		deviceContext->DrawIndexedInstanced(sub.tris * 3, count, sub.first * 3, 0, 0);
	}
}
