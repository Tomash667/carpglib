#include "Pch.h"
#include "SkyboxShader.h"
#include "Render.h"
#include "Mesh.h"
#include "Camera.h"
#include "DirectX.h"

//=================================================================================================
SkyboxShader::SkyboxShader() : vertex_shader(nullptr), pixel_shader(nullptr), layout(nullptr), vs_globals(nullptr), sampler(nullptr)
{
}

//=================================================================================================
void SkyboxShader::OnInit()
{
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	app::render->CreateShader("gui.hlsl", desc, countof(desc), vertex_shader, pixel_shader, layout);
	vs_globals = app::render->CreateConstantBuffer(sizeof(Matrix));
	sampler = app::render->CreateSampler(Render::TEX_ADR_CLAMP);



	/*effect = app::render->CompileShader("skybox.fx");

	tech = effect->GetTechniqueByName("skybox");
	assert(tech);

	hMatCombined = effect->GetParameterByName(nullptr, "matCombined");
	hTex = effect->GetParameterByName(nullptr, "tex0");
	assert(hMatCombined && hTex);*/
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
	IDirect3DDevice9* device = app::render->GetDevice();

	app::render->SetAlphaTest(false);
	app::render->SetAlphaBlend(false);
	app::render->SetNoCulling(false);
	app::render->SetNoZWrite(true);

	uint passes;
	Matrix m1 = Matrix::Translation(camera.from) * camera.mat_view_proj;

	V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(mesh.vertex_decl)));
	V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
	V(device->SetIndices(mesh.ib));

	V(effect->SetTechnique(tech));
	V(effect->SetMatrix(hMatCombined, (D3DXMATRIX*)&m1));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	for(vector<Mesh::Submesh>::iterator it = mesh.subs.begin(), end = mesh.subs.end(); it != end; ++it)
	{
		V(effect->SetTexture(hTex, it->tex->tex));
		V(effect->CommitChanges());
		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, it->min_ind, it->n_ind, it->first * 3, it->tris));
	}

	V(effect->EndPass());
	V(effect->End());
}
