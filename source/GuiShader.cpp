#include "Pch.h"
#include "GuiShader.h"
#include "Render.h"
#include "DirectX.h"

//=================================================================================================
GuiShader::GuiShader() {}
FIXME;
/*: effect(nullptr), vb(nullptr), vb2(nullptr)
{
}*/

void GuiShader::OnInit()
{

}

void GuiShader::OnRelease()
{

}

//=================================================================================================
/*void GuiShader::OnInit()
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

	// 
	// create pixel texture
	V(D3DXCreateTexture(device, 1, 1, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tPixel));
	D3DLOCKED_RECT lock;
	V(tPixel->LockRect(0, &lock, nullptr, 0));
	*((DWORD*)lock.pBits) = Color(255, 255, 255).value;
	V(tPixel->UnlockRect(0));
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
}*/
