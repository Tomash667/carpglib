#include "Pch.h"
#include "GlowShader.h"

#include "Camera.h"
#include "DirectX.h"
#include "Mesh.h"
#include "PostfxShader.h"
#include "Render.h"
#include "RenderTarget.h"
#include "SceneNode.h"

struct VsGlobals
{
	Matrix matCombined;
	Matrix matBones[Mesh::MAX_BONES];
};

struct PsGlobals
{
	Vec4 color;
};

//=================================================================================================
GlowShader::GlowShader(PostfxShader* postfxShader) : postfxShader(postfxShader), deviceContext(app::render->GetDeviceContext()), vertexShaderMesh(nullptr),
vertexShaderAni(nullptr), pixelShader(nullptr), layoutMesh(nullptr), layoutMeshTangent(nullptr), layoutMeshWeight(nullptr), layoutMeshTangentWeight(nullptr),
layoutAni(nullptr), layoutAniTangent(nullptr), vsGlobals(nullptr), psGlobals(nullptr)
{
}

//=================================================================================================
void GlowShader::OnInit()
{
	ID3DBlob* vsBlob;

	Render::ShaderParams params;
	params.name = "glow";
	params.vertexShader = &vertexShaderMesh;
	params.vsEntry = "VsMesh";
	params.vsBlob = &vsBlob;
	params.pixelShader = &pixelShader;
	app::render->CreateShader(params);
	layoutMesh = app::render->CreateInputLayout(VDI_DEFAULT, vsBlob, "GlowMeshLayout");
	layoutMeshTangent = app::render->CreateInputLayout(VDI_TANGENT, vsBlob, "GlowMeshTangentLayout");
	layoutMeshWeight = app::render->CreateInputLayout(VDI_ANIMATED, vsBlob, "GlowMeshWeightLayout");
	layoutMeshTangentWeight = app::render->CreateInputLayout(VDI_ANIMATED_TANGENT, vsBlob, "GlowMeshTangentWeightLayout");
	vsBlob->Release();

	params.vertexShader = &vertexShaderAni;
	params.vsEntry = "VsAni";
	params.pixelShader = nullptr;
	app::render->CreateShader(params);
	layoutAni = app::render->CreateInputLayout(VDI_ANIMATED, vsBlob, "GlowAniLayout");
	layoutAniTangent = app::render->CreateInputLayout(VDI_ANIMATED_TANGENT, vsBlob, "GlowAniTangentLayout");
	vsBlob->Release();

	vsGlobals = app::render->CreateConstantBuffer(sizeof(VsGlobals), "GlowVsGlobals");
	psGlobals = app::render->CreateConstantBuffer(sizeof(PsGlobals), "GlowPsGlobals");
}

//=================================================================================================
void GlowShader::OnRelease()
{
	SafeRelease(vertexShaderMesh);
	SafeRelease(vertexShaderAni);
	SafeRelease(pixelShader);
	SafeRelease(layoutMesh);
	SafeRelease(layoutMeshTangent);
	SafeRelease(layoutMeshWeight);
	SafeRelease(layoutMeshTangentWeight);
	SafeRelease(layoutAni);
	SafeRelease(layoutAniTangent);
	SafeRelease(vsGlobals);
	SafeRelease(psGlobals);
}

//=================================================================================================
void GlowShader::Draw(Camera& camera, const vector<GlowNode>& glowNodes, bool usePostfx)
{
	// resolve MS if required
	RenderTarget* targetScene = postfxShader->RequestActiveTarget();
	targetScene = postfxShader->ResolveTarget(targetScene);

	// set render target, reuse depth from normal scene
	RenderTarget* targetGlow = postfxShader->GetTarget(true);
	ID3D11RenderTargetView* renderTargetView = targetGlow->GetRenderTargetView();
	deviceContext->OMSetRenderTargets(1, &renderTargetView, app::render->GetDepthStencilView());
	deviceContext->ClearRenderTargetView(renderTargetView, (const float*)Vec4::Zero);

	// draw glow nodes to texture
	DrawGlowNodes(camera, glowNodes);

	// resolve MS if required
	targetGlow = postfxShader->ResolveTarget(targetGlow);

	// apply effects
	const float baseRange = 2.5f;
	const float rangeX = (baseRange / 1024.f);
	const float rangeY = (baseRange / 768.f);
	vector<PostEffect> effects;
	// blur by X
	PostEffect effect;
	effect.id = POSTFX_BLUR_X;
	effect.power = 1.f;
	effect.skill = Vec4(rangeX, rangeY, 0, 0);
	effects.push_back(effect);
	// blur by Y
	effect.id = POSTFX_BLUR_Y;
	effects.push_back(effect);
	// mask glow & blur to keep only outline
	effect.id = POSTFX_MASK;
	effect.tex = targetGlow->tex;
	effects.push_back(effect);
	postfxShader->Prepare();
	RenderTarget* targetBlurred = postfxShader->Draw(effects, targetGlow, false);
	postfxShader->FreeTarget(targetGlow);

	// combine render targets
	RenderTarget* targetFinal;
	if(usePostfx)
		targetFinal = postfxShader->GetTarget(false);
	else
		targetFinal = nullptr;
	postfxShader->Merge(targetScene, targetBlurred, targetFinal);
	postfxShader->FreeTarget(targetScene);
	postfxShader->FreeTarget(targetBlurred);
}

//=================================================================================================
void GlowShader::DrawGlowNodes(Camera& camera, const vector<GlowNode>& glowNodes)
{
	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_READ);
	app::render->SetRasterState(Render::RASTER_NORMAL);

	// setup shader
	deviceContext->VSSetConstantBuffers(0, 1, &vsGlobals);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &psGlobals);
	ID3D11SamplerState* sampler = app::render->GetSampler();
	deviceContext->PSSetSamplers(0, 1, &sampler);

	// draw all meshes
	VertexDeclarationId prevDecl = VDI_POS;
	Color prevColor = Color::None;
	bool prevAnimated = false;

	for(const GlowNode& glow : glowNodes)
	{
		// set vertex shader & layout
		Mesh* mesh = glow.node->mesh;
		const bool isAnimated = IsSet(glow.node->flags, SceneNode::F_ANIMATED);
		if(prevDecl != mesh->vertexDecl || prevAnimated != isAnimated)
		{
			ID3D11InputLayout* layout;
			switch(mesh->vertexDecl)
			{
			default:
			case VDI_DEFAULT:
				layout = layoutMesh;
				break;
			case VDI_TANGENT:
				layout = layoutMeshTangent;
				break;
			case VDI_ANIMATED:
				layout = isAnimated ? layoutAni : layoutMeshWeight;
				break;
			case VDI_ANIMATED_TANGENT:
				layout = isAnimated ? layoutAniTangent : layoutMeshTangentWeight;
				break;
			}
			deviceContext->VSSetShader(isAnimated ? vertexShaderAni : vertexShaderMesh, nullptr, 0);
			deviceContext->IASetInputLayout(layout);
			prevDecl = mesh->vertexDecl;
			prevAnimated = isAnimated;
		}

		// set vertex shader globals
		{
			ResourceLock lock(vsGlobals);
			VsGlobals& vsg = *lock.Get<VsGlobals>();
			vsg.matCombined = (glow.node->mat * camera.matViewProj).Transpose();
			if(isAnimated)
			{
				for(uint i = 0; i < glow.node->meshInst->mesh->head.n_bones; ++i)
					vsg.matBones[i] = glow.node->meshInst->matBones[i].Transpose();
			}
		}

		// set color
		if(glow.color != prevColor)
		{
			ResourceLock lock(psGlobals);
			lock.Get<PsGlobals>()->color = glow.color;
			prevColor = glow.color;
		}

		// set mesh
		uint stride = mesh->vertexSize, offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, &mesh->vb, &stride, &offset);
		deviceContext->IASetIndexBuffer(mesh->ib, DXGI_FORMAT_R16_UINT, 0);

		// draw mesh
		for(Mesh::Submesh& sub : mesh->subs)
		{
			deviceContext->PSSetShaderResources(0, 1, &sub.tex->tex);
			deviceContext->DrawIndexed(sub.tris * 3, sub.first * 3, 0);
		}
	}
}
