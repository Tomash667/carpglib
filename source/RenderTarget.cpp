#include "EnginePch.h"
#include "EngineCore.h"
#include "RenderTarget.h"
#include "Render.h"
#include "File.h"
#include "DirectX.h"

SURFACE RenderTarget::GetSurface()
{
	if(!surf)
	{
		tmp_surf = true;
		V(tex.tex->GetSurfaceLevel(0, &surf));
	}
	return surf;
}

void RenderTarget::SaveToFile(cstring filename)
{
	SURFACE s;
	if(surf)
		s = surf;
	else
		V(tex.tex->GetSurfaceLevel(0, &s));

	V(D3DXSaveSurfaceToFile(filename, D3DXIFF_JPG, s, nullptr, nullptr));

	if(!surf)
		s->Release();
}

uint RenderTarget::SaveToFile(FileWriter& f)
{
	SURFACE s;
	if(surf)
		s = surf;
	else
		V(tex.tex->GetSurfaceLevel(0, &s));

	LPD3DXBUFFER buffer;
	V(D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_JPG, s, nullptr, nullptr));
	uint size = buffer->GetBufferSize();
	f << size;
	f.Write(buffer->GetBufferPointer(), size);
	buffer->Release();

	if(!surf)
		s->Release();

	return size;
}

void RenderTarget::Resize(const Int2& new_size)
{
	SafeRelease(tex.tex);
	SafeRelease(surf);
	size = new_size;
	app::render->CreateRenderTargetTexture(this);
}

void RenderTarget::FreeSurface()
{
	if(tmp_surf)
	{
		surf->Release();
		surf = nullptr;
		tmp_surf = false;
	}
}
