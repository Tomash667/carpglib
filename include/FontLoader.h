#pragma once

//-----------------------------------------------------------------------------
class FontLoader
{
public:
	FontLoader();
	Font* Load(cstring name, int size, int weight, int outline);

private:
	void InitGdi();
	Font* LoadInternal(cstring name, int size, int weight, int outline);
	TEX CreateFontTexture(Font* font, const Int2& tex_size, int outline, int padding, void* winapi_font);

	ID3D11Device* device;
	bool gdi_initialized;
};
