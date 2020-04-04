#include "Pch.h"
#include "Texture.h"
#include "Render.h"
#include "DirectX.h"

//=================================================================================================
Texture::~Texture()
{
	SafeRelease(tex);
}

//=================================================================================================
void Texture::ResizeImage(Int2& new_size, Int2& img_size, Vec2& scale)
{
	img_size = GetSize();
	if(new_size == Int2(0, 0))
	{
		new_size = img_size;
		scale = Vec2(1, 1);
	}
	else if(new_size == img_size)
		scale = Vec2(1, 1);
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
TextureLock::TextureLock(TEX tex) : tex(tex)
{
	assert(tex);

	tex->GetResource(reinterpret_cast<ID3D11Resource**>(&res));

	D3D11_MAPPED_SUBRESOURCE resource;
	app::render->GetDeviceContext()->Map(res, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	data = static_cast<byte*>(resource.pData);
	pitch = resource.RowPitch;
}

//=================================================================================================
TextureLock::~TextureLock()
{
	if(res)
	{
		app::render->GetDeviceContext()->Unmap(res, 0);
		res->Release();
	}
}

//=================================================================================================
void TextureLock::Fill(Color color)
{
	D3D11_TEXTURE2D_DESC desc;
	res->GetDesc(&desc);
	for(uint y = 0; y < desc.Height; ++y)
	{
		uint* line = operator [](y);
		for(uint x = 0; x < desc.Width; ++x)
		{
			*line = color;
			++line;
		}
	}
	GenerateMipSubLevels();
}

//=================================================================================================
void TextureLock::GenerateMipSubLevels()
{
	assert(res);
	ID3D11DeviceContext* deviceContext = app::render->GetDeviceContext();
	deviceContext->Unmap(res, 0);
	deviceContext->GenerateMips(tex);
	res->Release();
	res = nullptr;
}
