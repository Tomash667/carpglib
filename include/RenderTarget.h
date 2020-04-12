#pragma once

//-----------------------------------------------------------------------------
#include "Texture.h"

//-----------------------------------------------------------------------------
class RenderTarget : public Texture
{
	friend class Render;

public:
	~RenderTarget();
	//void SaveToFile(cstring filename);
	//uint SaveToFile(FileWriter& f);
	//void Resize(const Int2& new_size);
	//void FreeSurface();
	//SURFACE GetSurface();
	//const Int2& GetSize() const { return size; }

private:
	ID3D11RenderTargetView* renderTarget;
	ID3D11DepthStencilView* depthStencilView;
	Int2 size;
};
