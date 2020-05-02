#include "Pch.h"
#include "RenderTarget.h"

#include "DirectX.h"

//=================================================================================================
RenderTarget::~RenderTarget()
{
	SafeRelease(renderTargetView);
	SafeRelease(depthStencilView);
}
