#pragma once

//-----------------------------------------------------------------------------
#include "Texture.h"

//-----------------------------------------------------------------------------
class RenderTarget : public Texture
{
	friend class Render;

public:
	~RenderTarget();
	const Int2& GetSize() const { return size; }
	ID3D11DepthStencilView* GetDepthStencilView() { return depthStencilView; }
	ID3D11RenderTargetView* GetRenderTargetView() { return renderTargetView; }

private:
	ID3D11RenderTargetView* renderTargetView;
	ID3D11DepthStencilView* depthStencilView;
	Int2 size;
	bool useWindowSize;
};
