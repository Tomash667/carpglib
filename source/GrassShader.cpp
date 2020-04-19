#include "Pch.h"
#include "GrassShader.h"
#include "Render.h"
#include "Mesh.h"
#include "Camera.h"
#include "Scene.h"
#include "SceneManager.h"
#include "DirectX.h"

struct VsGlobals
{
	Matrix matViewProj;
};

struct PsGlobals
{
	Vec4 fogColor;
	Vec4 fogParam;
	Vec3 ambientColor;
};

//=================================================================================================
GrassShader::GrassShader() : deviceContext(app::render->GetDeviceContext()), vertexShader(nullptr), pixelShader(nullptr), layout(nullptr),
vsGlobals(nullptr), psGlobals(nullptr), sampler(nullptr), vb(nullptr), vbSize(0)
{
}

//=================================================================================================
void GrassShader::OnInit()
{
	app::render->CreateShader("grass.hlsl", VDI_GRASS, vertexShader, pixelShader, layout);
	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals));
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals));
	sampler = app::render->CreateSampler();
}

//=================================================================================================
void GrassShader::OnRelease()
{
	SafeRelease(vertexShader);
	SafeRelease(pixelShader);
	SafeRelease(layout);
	SafeRelease(vsGlobals);
	SafeRelease(psGlobals);
	SafeRelease(sampler);
	SafeRelease(vb);
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
	deviceContext->IASetInputLayout(layout);

	// setup stream source for instancing
	/*V(device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1));
	V(device->SetStreamSource(1, vb, 0, sizeof(Matrix)));
	V(device->SetVertexDeclaration(vertex_decl));

	// set effect
	uint passes;
	V(effect->SetTechnique(tech));
	V(effect->SetVector(h_fog_color, (D3DXVECTOR4*)&scene->GetFogColor()));
	V(effect->SetVector(h_fog_params, (D3DXVECTOR4*)&scene->GetFogParams()));
	V(effect->SetVector(h_ambient, (D3DXVECTOR4*)&scene->GetAmbientColor()));
	V(effect->SetMatrix(h_view_proj, (D3DXMATRIX*)&camera->mat_view_proj));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));*/
}

//=================================================================================================
void GrassShader::Draw(Mesh* mesh, const vector<const vector<Matrix>*>& patches, uint count)
{
	assert(mesh->vertex_decl == VDI_DEFAULT);

	uint size = sizeof(VDefault), offset = 0;
	deviceContext->IASetInputLayout(layout);
	deviceContext->IASetVertexBuffers(0, 1, &vb, &size, &offset);
	deviceContext->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);

	// setup instancing data
	Matrix* m;
	V(vb->Lock(0, 0, (void**)&m, D3DLOCK_DISCARD));
	int index = 0;
	for(vector< const vector<Matrix>* >::const_iterator it = patches.begin(), end = patches.end(); it != end; ++it)
	{
		const vector<Matrix>& vm = **it;
		memcpy(&m[index], &vm[0], sizeof(Matrix)*vm.size());
		index += vm.size();
	}
	V(vb->Unlock());

	// setup stream source for mesh
	V(device->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | count));
	V(device->SetStreamSource(0, mesh->vb, 0, mesh->vertex_size));
	V(device->SetIndices(mesh->ib));

	// draw
	for(int i = 0; i < mesh->head.n_subs; ++i)
	{
		V(effect->SetTexture(h_tex, mesh->subs[i].tex->tex));
		V(effect->CommitChanges());
		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
	}
}

//=================================================================================================
void GrassShader::End()
{
	V(effect->EndPass());
	V(effect->End());

	// restore vertex stream frequency
	V(device->SetStreamSourceFreq(0, 1));
	V(device->SetStreamSourceFreq(1, 1));
}*/
FIXME;
