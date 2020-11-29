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
void Texture::ResizeImage(Int2& new_size, Int2& img_size, Vec2& scale)
{
	img_size = GetSize();
	if(new_size == Int2::Zero)
	{
		new_size = img_size;
		scale = Vec2::One;
	}
	else if(new_size == img_size)
		scale = Vec2::One;
	else
		scale = Vec2(float(new_size.x) / img_size.x, float(new_size.y) / img_size.y);
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
Vec2 Texture::GetScale(const Int2& imgSize, const Int2& requiredSize)
{
	assert(imgSize.x > 0 && imgSize.y > 0 && requiredSize.x > 0 && requiredSize.y > 0);
	if(imgSize == requiredSize)
		return Vec2::One;
	else
		return Vec2(float(requiredSize.x) / imgSize.x, float(requiredSize.y) / imgSize.y);
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
