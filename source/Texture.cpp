#include "EnginePch.h"
#include "EngineCore.h"
#include "Texture.h"
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
	D3DSURFACE_DESC desc;
	V(tex->GetLevelDesc(0, &desc));
	return Int2(desc.Width, desc.Height);
}

//=================================================================================================
TextureLock::TextureLock(TEX tex) : tex(tex)
{
	assert(tex);
	D3DLOCKED_RECT lock;
	V(tex->LockRect(0, &lock, nullptr, 0));
	data = static_cast<byte*>(lock.pBits);
	pitch = lock.Pitch;
}

//=================================================================================================
TextureLock::~TextureLock()
{
	if(tex)
		V(tex->UnlockRect(0));
}

//=================================================================================================
void TextureLock::Fill(Color color)
{
	Int2 size = Texture::GetSize(tex);
	for(int y = 0; y < size.y; ++y)
	{
		uint* line = operator [](y);
		for(int x = 0; x < size.x; ++x)
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
	assert(tex);
	V(tex->UnlockRect(0));
	tex->GenerateMipSubLevels();
	tex = nullptr;
}
