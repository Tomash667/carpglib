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
vsGlobals(nullptr), psGlobals(nullptr), sampler(nullptr)
{
}

//=================================================================================================
void GrassShader::OnInit()
{
	FIXME;
	//app::render->CreateShader("grass.hlsl", VDI_GRASS, vertexShader, pixelShader, layout);
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
}

//=================================================================================================
/*void GrassShader::OnInit()
{
	effect = app::render->CompileShader("grass.fx");

	tech = effect->GetTechniqueByName("grass");
	assert(tech);

	h_view_proj = effect->GetParameterByName(nullptr, "matViewProj");
	h_tex = effect->GetParameterByName(nullptr, "texDiffuse");
	h_fog_color = effect->GetParameterByName(nullptr, "fogColor");
	h_fog_params = effect->GetParameterByName(nullptr, "fogParam");
	h_ambient = effect->GetParameterByName(nullptr, "ambientColor");
	assert(h_view_proj && h_tex && h_fog_color && h_fog_params && h_ambient);
}

//=================================================================================================
void GrassShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
	SafeRelease(vb);
	vb_size = 0;
}

//=================================================================================================
void GrassShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void GrassShader::OnRelease()
{
	SafeRelease(effect);
	SafeRelease(vb);
}

//=================================================================================================
void GrassShader::Begin(Scene* scene, Camera* camera, uint max_size)
{
	assert(scene && camera);

	app::render->SetAlphaBlend(false);
	app::render->SetAlphaTest(true);
	app::render->SetNoCulling(true);
	app::render->SetNoZWrite(false);

	// create vertex buffer if existing is too small
	if(!vb || vb_size < max_size)
	{
		SafeRelease(vb);
		if(vb_size < max_size)
			vb_size = max_size;
		V(device->CreateVertexBuffer(sizeof(Matrix)*vb_size, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vb, nullptr));
	}

	// setup stream source for instancing
	V(device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1));
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
	V(effect->BeginPass(0));
}

//=================================================================================================
void GrassShader::Draw(Mesh* mesh, const vector<const vector<Matrix>*>& patches, uint count)
{
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
