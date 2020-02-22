#include "Pch.h"
#include "GuiShader.h"
#include "Render.h"
#include "DirectX.h"

//=================================================================================================
GuiShader::GuiShader() : effect(nullptr), vb(nullptr), vb2(nullptr)
{
}

//=================================================================================================
void GuiShader::OnInit()
{
	effect = app::render->CompileShader("gui.fx");

	techTex = effect->GetTechniqueByName("techTex");
	techColor = effect->GetTechniqueByName("techColor");
	techGrayscale = effect->GetTechniqueByName("techGrayscale");
	assert(techTex && techColor && techGrayscale);

	hSize = effect->GetParameterByName(nullptr, "size");
	hTex = effect->GetParameterByName(nullptr, "tex0");
	assert(hSize && hTex);

	CreateVertexBuffer();
}

//=================================================================================================
void GuiShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
	SafeRelease(vb);
	SafeRelease(vb2);
}

//=================================================================================================
void GuiShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
	CreateVertexBuffer();
}

//=================================================================================================
void GuiShader::OnRelease()
{
	SafeRelease(effect);
	SafeRelease(vb);
	SafeRelease(vb2);
}

//=================================================================================================
void GuiShader::CreateVertexBuffer()
{
	IDirect3DDevice9* device = app::render->GetDevice();
	V(device->CreateVertexBuffer(sizeof(VParticle) * 6 * 256, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &vb, nullptr));
	V(device->CreateVertexBuffer(sizeof(VParticle) * 6 * 256, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &vb2, nullptr));
}
