#include "Pch.h"
#include "DepthShader.h"

#include "Camera.h"
#include "DirectX.h"
#include "Render.h"
#include "SceneNode.h"

struct VsLocals
{
	Matrix matCombined;
	Matrix matBones[Mesh::MAX_BONES];
};

//=================================================================================================
DepthShader::DepthShader() : deviceContext(app::render->GetDeviceContext()), vertexShaderMesh(nullptr), vertexShaderAni(nullptr), pixelShader(nullptr),
layoutMesh(nullptr), layoutMeshTangent(nullptr), layoutMeshWeight(nullptr), layoutMeshTangentWeight(nullptr), layoutAni(nullptr), layoutAniTangent(nullptr),
vsLocals(nullptr)
{
}

//=================================================================================================
void DepthShader::OnInit()
{
	ID3DBlob* vsBlob;

	Render::ShaderParams params;
	params.name = "depth";
	params.decl = VDI_POS;
	params.vertexShader = &vertexShaderMesh;
	params.vsEntry = "VsMesh";
	params.pixelShader = &pixelShader;
	params.vsBlob = &vsBlob;
	app::render->CreateShader(params);
	layoutMesh = app::render->CreateInputLayout(VDI_DEFAULT, vsBlob, "DepthMeshLayout");
	layoutMeshTangent = app::render->CreateInputLayout(VDI_TANGENT, vsBlob, "DepthMeshTangentLayout");
	layoutMeshWeight = app::render->CreateInputLayout(VDI_ANIMATED, vsBlob, "DepthMeshWeightLayout");
	layoutMeshTangentWeight = app::render->CreateInputLayout(VDI_ANIMATED_TANGENT, vsBlob, "DepthMeshTangentWeightLayout");
	vsBlob->Release();

	params.vertexShader = &vertexShaderAni;
	params.vsEntry = "VsAni";
	params.pixelShader = nullptr;
	app::render->CreateShader(params);
	layoutAni = app::render->CreateInputLayout(VDI_ANIMATED, vsBlob, "DepthAniLayout");
	layoutAniTangent = app::render->CreateInputLayout(VDI_ANIMATED_TANGENT, vsBlob, "DepthAniTangentLayout");
	vsBlob->Release();

	vsLocals = app::render->CreateConstantBuffer(sizeof(VsLocals), "DepthVsLocals");
}

//=================================================================================================
void DepthShader::OnRelease()
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
	SafeRelease(vsLocals);
}

//=================================================================================================
void DepthShader::Prepare(const Camera& camera)
{
	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_YES);
	app::render->SetRasterState(Render::RASTER_NORMAL);

	// setup shader
	deviceContext->VSSetConstantBuffers(0, 1, &vsLocals);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);

	matViewProj = camera.matViewProj;
}

//=================================================================================================
void DepthShader::Draw(SceneNode* node)
{
	// set vertex shader & layout
	Mesh* mesh = node->mesh;
	const bool isAnimated = IsSet(node->flags, SceneNode::F_ANIMATED);
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

	// set constants
	{
		ResourceLock lock(vsLocals);
		VsLocals& vsl = *lock.Get<VsLocals>();
		vsl.matCombined = (node->mat * matViewProj).Transpose();
		if(isAnimated)
		{
			const vector<Matrix>& matBones = node->meshInst->GetBoneMatrices();
			for(uint i = 0, count = matBones.size(); i < count; ++i)
				vsl.matBones[i] = matBones[i].Transpose();
		}
	}

	// set mesh
	uint stride = mesh->vertexSize, offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &mesh->vb, &stride, &offset);
	deviceContext->IASetIndexBuffer(mesh->ib, DXGI_FORMAT_R16_UINT, 0);

	// draw mesh
	for(Mesh::Submesh& sub : mesh->subs)
		deviceContext->DrawIndexed(sub.tris * 3, sub.first * 3, 0);
}


//=================================================================================================
void DepthShader::DrawCustom(ID3D11Buffer* vb, ID3D11Buffer* ib, uint startIndex, uint indexCount)
{
	// set vertex shader & layout
	deviceContext->VSSetShader(vertexShaderMesh, nullptr, 0);
	deviceContext->IASetInputLayout(layoutMesh);

	// set constants
	{
		ResourceLock lock(vsLocals);
		VsLocals& vsl = *lock.Get<VsLocals>();
		vsl.matCombined = (Matrix::IdentityMatrix * matViewProj).Transpose();
	}

	// set mesh
	uint stride = sizeof(VDefault), offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	deviceContext->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);

	// draw mesh
	deviceContext->DrawIndexed(indexCount, startIndex, 0);
}
