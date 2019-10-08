#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Texture : public Resource
{
	static constexpr ResourceType Type = ResourceType::Texture;

	TEX tex;

	Texture() : tex(nullptr) {}
	~Texture();
	void ResizeImage(Int2& new_size, Int2& img_size, Vec2& scale);
	Int2 GetSize() const { return GetSize(tex); }
	static Int2 GetSize(TEX tex);
};

//-----------------------------------------------------------------------------
struct TexOverride
{
	explicit TexOverride(Texture* diffuse) : diffuse(diffuse) {}

	Texture* diffuse;
};

//-----------------------------------------------------------------------------
struct TextureLock
{
	TextureLock(TEX tex);
	~TextureLock();
	uint* operator [] (uint row) { return (uint*)(data + pitch * row); }
	void GenerateMipSubLevels();

private:
	TEX tex;
	byte* data;
	int pitch;
};
