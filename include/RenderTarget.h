#pragma once

//-----------------------------------------------------------------------------
#include "Texture.h"

//-----------------------------------------------------------------------------
class RenderTarget : public Texture
{
	friend class Render;

	RenderTarget() : tmp_surf(false) {}
public:
	//void SaveToFile(cstring filename);
	//uint SaveToFile(FileWriter& f);
	void Resize(const Int2& new_size);
	void FreeSurface();
	//SURFACE GetSurface();
	const Int2& GetSize() const { return size; }

private:
	//SURFACE surf;
	FIXME; // ^
	Int2 size; // ? required
	bool tmp_surf;
};
