#include "Pch.h"
#include "Texture.h"

#include "DirectX.h"
#include "Render.h"

//=================================================================================================
Texture::~Texture()
{
	SafeRelease(tex);
}

//=================================================================================================
void Texture::Release()
{
	SafeRelease(tex);
	state = ResourceState::NotLoaded;
}

//=================================================================================================
void Texture::ResizeImage(Int2& newSize, Int2& imgSize, Vec2& scale)
{
	imgSize = GetSize();
	if(newSize == Int2(0, 0))
	{
		newSize = imgSize;
		scale = Vec2(1, 1);
	}
	else if(newSize == imgSize)
		scale = Vec2(1, 1);
	else
		scale = Vec2(float(newSize.x) / imgSize.x, float(newSize.y) / imgSize.y);
}

//=================================================================================================
Int2 Texture::GetSize(TEX tex)
{
	assert(tex);
	ID3D11Texture2D* res;
	tex->GetResource(reinterpret_cast<ID3D11Resource**>(&res));
	D3D11_TEXTURE2D_DESC desc;
	res->GetDesc(&desc);
	Int2 size(desc.Width, desc.Height);
	res->Release();
	return size;
}

//=================================================================================================
DynamicTexture::~DynamicTexture()
{
	SafeRelease(tex);
	SafeRelease(texResource);
	SafeRelease(texStaging);
}

//=================================================================================================
void DynamicTexture::Lock()
{
	D3D11_MAPPED_SUBRESOURCE subresource;
	V(app::render->GetDeviceContext()->Map(texStaging, 0, D3D11_MAP_READ_WRITE, 0, &subresource));
	data = static_cast<byte*>(subresource.pData);
	pitch = subresource.RowPitch;
}

//=================================================================================================
void DynamicTexture::Unlock(bool generateMipmaps)
{
	ID3D11DeviceContext* deviceContext = app::render->GetDeviceContext();
	deviceContext->Unmap(texStaging, 0);
	deviceContext->CopyResource(texResource, texStaging);
	if(generateMipmaps)
		deviceContext->GenerateMips(tex);
}
