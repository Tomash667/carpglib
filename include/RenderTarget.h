#pragma once

//-----------------------------------------------------------------------------
#include "Texture.h"

//-----------------------------------------------------------------------------
class RenderTarget : public Texture
{
	friend class Render;

public:
	enum Flags
	{
		F_NO_DEPTH = 1 << 0,
		F_NO_DRAW = 1 << 1,
		F_NO_MS = 1 << 2,
		F_USE_WINDOW_SIZE = 1 << 3
	};

	~RenderTarget();
	int GetFlags() const { return flags; }
	const Int2& GetSize() const { return size; }
	ID3D11DepthStencilView* GetDepthStencilView() { return depthStencilView; }
	ID3D11RenderTargetView* GetRenderTargetView() { return renderTargetView; }

private:
	ID3D11RenderTargetView* renderTargetView;
	ID3D11DepthStencilView* depthStencilView;
	Int2 size;
	int flags;
};
