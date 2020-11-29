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
	void Release();
	void ResizeImage(Int2& new_size, Int2& img_size, Vec2& scale);
	Int2 GetSize() const { return GetSize(tex); }
	static Int2 GetSize(TEX tex);
	static Vec2 GetScale(const Int2& imgSize, const Int2& requiredSize);
};

//-----------------------------------------------------------------------------
struct TexOverride
{
	explicit TexOverride(Texture* diffuse = nullptr) : diffuse(diffuse), normal(nullptr), specular(nullptr) {}
	int GetIndex() const
	{
		return (normal ? 2 : 0) + (specular ? 1 : 0);
	}

	TexturePtr diffuse, normal, specular;
};

//-----------------------------------------------------------------------------
struct DynamicTexture : public Texture
{
	friend class Render;

	~DynamicTexture();
	void Lock();
	void Unlock(bool generateMipmaps = false);
	uint* operator [] (uint row) { return (uint*)(data + pitch * row); }

private:
	ID3D11Texture2D* texResource;
	ID3D11Texture2D* texStaging;
	byte* data;
	int pitch;
};
