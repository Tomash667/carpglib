#pragma once

//-----------------------------------------------------------------------------
class FontLoader
{
public:
	FontLoader();
	Font* Load(cstring name, int size, int weight, int outline);

private:
	Font* LoadInternal(cstring name, int size, int weight, int outline);
	TEX CreateFontTexture(Font* font, const Int2& texSize, int outline, int padding, void* winapiFont);

	ID3D11Device* device;
};
