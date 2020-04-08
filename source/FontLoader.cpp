#include "Pch.h"
#include "FontLoader.h"

#include "DirectX.h"
#include "Font.h"
#include "Render.h"
#include "Texture.h"

//=================================================================================================
FontLoader::FontLoader() : device(app::render->GetDevice())
{
}

//=================================================================================================
Font* FontLoader::Load(cstring name, int size, int weight, int outline)
{
	assert(name && size > 0 && InRange(weight, 1, 9) && outline >= 0);

	try
	{
		return LoadInternal(name, size, weight, outline);
	}
	catch(cstring err)
	{
		throw Format("Failed to load font '%s'(%d): %s", name, size, err);
	}
}

//=================================================================================================
Font* FontLoader::LoadInternal(cstring name, int size, int weight, int outline)
{
	HDC hdc = GetDC(nullptr);
	int logic_size = -MulDiv(size, 96, 72);

	// create winapi font
	HFONT winapiFont = CreateFontA(logic_size, 0, 0, 0, weight * 100, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name);
	if(!winapiFont)
	{
		DWORD error = GetLastError();
		ReleaseDC(nullptr, hdc);
		throw Format("Failed to create winapi font (%u).", error);
	}

	// get glyphs weights, font height
	int glyph_w[256];
	SelectObject(hdc, (HGDIOBJ)winapiFont);
	if(GetCharWidth32(hdc, 0, 255, glyph_w) == 0)
	{
		ABC abc[256];
		if(GetCharABCWidths(hdc, 0, 255, abc) == 0)
		{
			DWORD error = GetLastError();
			DeleteObject(winapiFont);
			ReleaseDC(nullptr, hdc);
			throw Format("Failed to get font glyphs (%u).", error);
		}
		for(int i = 0; i <= 255; ++i)
		{
			ABC& a = abc[i];
			glyph_w[i] = a.abcA + a.abcB + a.abcC;
		}
	}
	TEXTMETRIC tm;
	GetTextMetricsA(hdc, &tm);
	Ptr<Font> font;
	font->height = tm.tmHeight;
	ReleaseDC(nullptr, hdc);

	// calculate texture size
	const int padding = outline ? outline + 2 : 1;
	Int2 texSize(padding * 2, padding * 2 + font->height);
	for(int i = 32; i <= 255; ++i)
	{
		int width = glyph_w[i];
		if(width)
			texSize.x += width + padding;
	}
	texSize.x = NextPow2(texSize.x);
	texSize.y = NextPow2(texSize.y);

	// setup glyphs
	Int2 offset(padding, padding);
	for(int i = 0; i < 32; ++i)
		font->glyph[i].width = 0;
	for(int i = 32; i <= 255; ++i)
	{
		Font::Glyph& glyph = font->glyph[i];
		glyph.width = glyph_w[i];
		if(glyph.width)
		{
			glyph.uv.v1 = Vec2(float(offset.x) / texSize.x, float(offset.y) / texSize.y);
			glyph.uv.v2 = glyph.uv.v1 + Vec2(float(glyph.width) / texSize.x, float(font->height) / texSize.y);
			offset.x += glyph.width + padding;
		}
	}

	// create textures
	font->outline = outline;
	font->outline_shift = Vec2(float(outline) / texSize.x, float(outline) / texSize.y);
	font->tex = CreateFontTexture(font, texSize, 0, padding, winapiFont);
	if(outline > 0)
		font->tex_outline = CreateFontTexture(font, texSize, outline, padding, winapiFont);
	DeleteObject(winapiFont);

	// make tab size of 4 spaces
	Font::Glyph& tab = font->glyph['\t'];
	Font::Glyph& space = font->glyph[' '];
	tab.width = space.width * 4;;
	tab.uv = space.uv;

	// save textures to file
	/*app::render->SaveToFile(font->tex, Format("%s-%d.png", name, size), ImageFormat::PNG);
	if(outline > 0)
		app::render->SaveToFile(font->tex_outline, Format("%s-%d-outline.png", name, size), ImageFormat::PNG);*/

	return font.Pin();
}

//=================================================================================================
TEX FontLoader::CreateFontTexture(Font* font, const Int2& texSize, int outline, int padding, void* winapiFont)
{
	// create font
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = texSize.x;
	desc.Height = texSize.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

	ID3D11Texture2D* tex;
	HRESULT result = device->CreateTexture2D(&desc, nullptr, &tex);
	if(FAILED(result))
	{
		DeleteObject(winapiFont);
		throw Format("Failed to create texture (%ux%u, result %u).", texSize.x, texSize.y, result);
	}
	
	// get texture bitmap
	IDXGISurface1* surface;
	V(tex->QueryInterface(__uuidof(IDXGISurface1), (void**)&surface));
	HDC hdc;
	V(surface->GetDC(TRUE, &hdc));

	// create new bitmap
	HDC hdc2 = CreateCompatibleDC(hdc);
	BITMAPINFO bmpInfo = {};
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmpInfo.bmiHeader.biWidth = texSize.x;
	bmpInfo.bmiHeader.biHeight = -texSize.y;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	Color* bits;
	HBITMAP bmp = CreateDIBSection(hdc2, &bmpInfo, DIB_RGB_COLORS, (void**)&bits, 0, 0);
	SelectObject(hdc2, bmp);

	HFONT hfont = (HFONT)winapiFont;
	SelectObject(hdc2, hfont);
	SetBkMode(hdc2, 1);
	SetTextColor(hdc2, RGB(255, 255, 255));

	// draw glyphs on bitmap
	Int2 offset(padding, padding);
	wchar_t wc[4];
	char c[2];
	c[1] = 0;
	for(int i = 32; i <= 255; ++i)
	{
		Font::Glyph& glyph = font->glyph[i];
		if(glyph.width == 0)
			continue;

		c[0] = (char)i;
		size_t count;
		mbstowcs_s(&count, wc, c, 4);

		if(outline)
		{
			for(int j = 0; j < 8; ++j)
			{
				const float a = float(j) * PI / 4;
				int x = int(offset.x + outline * sin(a));
				int y = int(offset.y + outline * cos(a));
				TextOutA(hdc2, x, y, c, 1);
			}
		}
		else
			TextOutA(hdc2, offset.x, offset.y, c, 1);

		offset.x += glyph.width + padding;
	}

	// adjust alpha
	for(int y = 0; y < texSize.y; ++y)
	{
		for(int x = 0; x < texSize.x; ++x)
		{
			Color& c = bits[x + y * texSize.x];
			int alpha;
			if(c.r == 255)
				alpha = 255;
			else if(c.r < 50)
				alpha = 0;
			else
				alpha = (int)(0.004f * c.r * c.r + 0.0035f * c.r - 6.f);
			c.a = alpha;
		}
	}

	// copy to directx bitmap
	BLENDFUNCTION f;
	f.BlendOp = 0;
	f.BlendFlags = 0;
	f.AlphaFormat = 0;
	f.SourceConstantAlpha = 255;

	AlphaBlend(hdc, 0, 0, texSize.x, texSize.y, hdc2, 0, 0, texSize.x, texSize.y, f);
	DeleteObject(bmp);
	DeleteObject(hdc2);

	V(surface->ReleaseDC(nullptr));
	surface->Release();

	// create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

	TEX view;
	V(device->CreateShaderResourceView(tex, &SRVDesc, &view));
	tex->Release();

	return view;
}
