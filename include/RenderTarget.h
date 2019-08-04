#pragma once

//-----------------------------------------------------------------------------
#include "Texture.h"

//-----------------------------------------------------------------------------
class RenderTarget : public Texture
{
	friend class Render;

	RenderTarget() : tmp_surf(false) {}
	~RenderTarget() {}
public:
	void SaveToFile(cstring filename);
	uint SaveToFile(FileWriter& f);
	void FreeSurface();
	Texture* GetTexture() { return &tex; }
	SURFACE GetSurface();
	const Int2& GetSize() const { return size; }

private:
	Texture tex;
	SURFACE surf;
	Int2 size;
	bool tmp_surf;
};
