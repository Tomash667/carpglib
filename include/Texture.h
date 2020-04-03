#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"
#include "ManagedResource.h"

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
struct DynamicTexture : public Texture, public ManagedResource
{
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;

	Int2 size;
	delegate<void()> reload;
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
struct TextureLock
{
	TextureLock(TEX tex);
	TextureLock(Texture* tex) : TextureLock(tex->tex) {}
	~TextureLock();
	uint* operator [] (uint row) { return (uint*)(data + pitch * row); }
	void Fill(Color color);
	void GenerateMipSubLevels();

private:
	TEX tex;
	ID3D11Texture2D* res;
	byte* data;
	int pitch;
};
