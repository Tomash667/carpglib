#include "Pch.h"
#include "Render.h"
#include "RenderTarget.h"
#include "Engine.h"
#include "ShaderHandler.h"
#include "File.h"
#include "ManagedResource.h"
#include "DirectX.h"
#include <d3dcompiler.h>

Render* app::render;
static const DXGI_FORMAT DISPLAY_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

//=================================================================================================
Render::Render() : initialized(false), current_target(nullptr), /*current_surf(nullptr),*/ vsync(true),
lost_device(false), res_freed(false), shaders_dir("shaders"), refresh_hz(0), shader_version(-1), used_adapter(0), multisampling(0), multisampling_quality(0),

factory(nullptr), adapter(nullptr), swap_chain(nullptr), device(nullptr), deviceContext(nullptr), render_target(nullptr), depth_stencil_view(nullptr)
{
	/*for(int i = 0; i < VDI_MAX; ++i)
		vertex_decl[i] = nullptr;-*/
}

//=================================================================================================
Render::~Render()
{
	for(ShaderHandler* shader : shaders)
	{
		shader->OnRelease();
		delete shader;
	}
	for(ManagedResource* res : managed_res)
		res->OnRelease();
	//for(int i = 0; i < VDI_MAX; ++i)
	//	SafeRelease(vertex_decl[i]);
	//----------------------------
	SafeRelease(depth_stencil_view);
	SafeRelease(render_target);
	SafeRelease(swap_chain);
	SafeRelease(deviceContext);

	if(device)
	{
		// write to output directx leaks
#ifdef _DEBUG
		ID3D11Debug* debug;
		device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug);
		if(debug)
		{
			debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
			debug->Release();
		}
#endif
		device->Release();
	}

	SafeRelease(adapter);
	SafeRelease(factory);
}

//=================================================================================================
void Render::Init()
{
	wnd_size = app::engine->GetWindowSize();

	CreateAdapter();
	CreateDeviceAndSwapChain();
	CreateSizeDependentResources();

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// check shaders version
	/*D3DCAPS9 caps;
	hr = d3d->GetDeviceCaps(used_adapter, D3DDEVTYPE_HAL, &caps);
	if(FAILED(hr))
		throw Format("Render: Failed to GetDeviceCaps (%u)! Make sure that you have graphic card drivers installed.", hr);
	else if(D3DVS_VERSION(2, 0) > caps.VertexShaderVersion || D3DPS_VERSION(2, 0) > caps.PixelShaderVersion)
	{
		throw Format("Render: Too old graphic card! This game require vertex and pixel shader in version 2.0+. "
			"Your card support:\nVertex shader: %d.%d\nPixel shader: %d.%d",
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));
	}
	else
	{
		Info("Supported shader version vertex: %d.%d, pixel: %d.%d.",
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));

		int version = min(D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion));
		if(shader_version == -1 || shader_version > version || shader_version < 2)
			shader_version = version;

		Info("Using shader version %d.", shader_version);
	}

	// check texture types
	bool fullscreen = app::engine->IsFullscreen();
	hr = d3d->CheckDeviceType(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, BACKBUFFER_FORMAT, fullscreen ? FALSE : TRUE);
	if(FAILED(hr))
		throw Format("Render: Unsupported backbuffer type %s for display %s! (%d)", STRING(BACKBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDeviceFormat(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Render: Unsupported depth buffer type %s for display %s! (%d)", STRING(ZBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDepthStencilMatch(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DFMT_A8R8G8B8, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Render: Unsupported render target D3DFMT_A8R8G8B8 with display %s and depth buffer %s! (%d)",
		STRING(DISPLAY_FORMAT), STRING(BACKBUFFER_FORMAT), hr);

	// check multisampling
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels))
		&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels2)))
	{
		levels = min(levels, levels2);
		if(multisampling_quality < 0 || multisampling_quality >= (int)levels)
		{
			Warn("Render: Unavailable multisampling quality, changed to 0.");
			multisampling_quality = 0;
		}
	}
	else
	{
		Warn("Render: Your graphic card don't support multisampling x%d. Maybe it's only available in fullscreen mode. "
			"Multisampling was turned off.", multisampling);
		multisampling = 0;
		multisampling_quality = 0;
	}

	LogMultisampling();
	LogAndSelectResolution();

	// gather params
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	GatherParams(d3dpp);

	// available modes
	const DWORD mode[] = {
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		D3DCREATE_MIXED_VERTEXPROCESSING,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING
	};
	const cstring mode_str[] = {
		"hardware",
		"mixed",
		"software"
	};

	// try to create device in one of modes
	for(uint i = 0; i < 3; ++i)
	{
		DWORD sel_mode = mode[i];
		hr = d3d->CreateDevice(used_adapter, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, sel_mode, &d3dpp, &device);

		if(SUCCEEDED(hr))
		{
			Info("Render: Created direct3d device in %s mode.", mode_str[i]);
			break;
		}
	}

	// failed to create device
	if(FAILED(hr))
		throw Format("Render: Failed to create direct3d device (%d).", hr);

	// create sprite
	hr = D3DXCreateSprite(device, &sprite);
	if(FAILED(hr))
		throw Format("Render: Failed to create direct3dx sprite (%d).", hr);

	SetDefaultRenderState();
	CreateVertexDeclarations();*/

	initialized = true;
	Info("Render: Directx device created.");
}

//=================================================================================================
void Render::CreateAdapter()
{
	// create dxgi factory
	V(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory));

	// enumerate adapters
	IDXGIAdapter* tmp_adapter;
	for(int i = 0; factory->EnumAdapters(i, &tmp_adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC desc;
		V(tmp_adapter->GetDesc(&desc));
		Info("Render: Adapter %d: %s", i, ToString(desc.Description));
		if(used_adapter == i)
			adapter = tmp_adapter;
		else
			tmp_adapter->Release();
	}

	// fallback to first adapter
	if(!adapter)
	{
		Warn("Render: Invalid adapter %d, defaulting to 0.", used_adapter);
		used_adapter = 0;
		V(factory->EnumAdapters(0, &adapter));
	}
}

//=================================================================================================
void Render::CreateDeviceAndSwapChain()
{
	DXGI_SWAP_CHAIN_DESC swap_desc = {};
	swap_desc.BufferCount = 1;
	swap_desc.BufferDesc.Width = wnd_size.x;
	swap_desc.BufferDesc.Height = wnd_size.y;
	swap_desc.BufferDesc.Format = DISPLAY_FORMAT;
	swap_desc.BufferDesc.RefreshRate.Numerator = 0;
	swap_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_desc.OutputWindow = app::engine->GetWindowHandle();
	swap_desc.SampleDesc.Count = 1;
	swap_desc.SampleDesc.Quality = 0;
	swap_desc.Windowed = true;
	swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swap_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	int flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL feature_level;
	HRESULT result = D3D11CreateDeviceAndSwapChain(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, feature_levels, countof(feature_levels),
		D3D11_SDK_VERSION, &swap_desc, &swap_chain, &device, &feature_level, &deviceContext);
	if(FAILED(result))
		throw Format("Failed to create device and swap chain (%u).", result);

	// disable builtin alt+enter
	V(factory->MakeWindowAssociation(swap_desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES));
}

//=================================================================================================
void Render::CreateSizeDependentResources()
{
	CreateRenderTarget();
	CreateDepthStencilView();
	deviceContext->OMSetRenderTargets(1, &render_target, depth_stencil_view);
	SetViewport();
}

//=================================================================================================
void Render::CreateRenderTarget()
{
	HRESULT result;
	ID3D11Texture2D* back_buffer;
	result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
	if(FAILED(result))
		throw Format("Failed to get back buffer (%u).", result);

	// Create the render target view with the back buffer pointer.
	result = device->CreateRenderTargetView(back_buffer, NULL, &render_target);
	if(FAILED(result))
		throw Format("Failed to create render target view (%u).", result);

	// Release pointer to the back buffer as we no longer need it.
	back_buffer->Release();
}

//=================================================================================================
void Render::CreateDepthStencilView()
{
	// create depth buffer texture
	D3D11_TEXTURE2D_DESC tex_desc = {};

	tex_desc.Width = wnd_size.x;
	tex_desc.Height = wnd_size.y;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = 0;

	ID3D11Texture2D* depth_tex;
	V(device->CreateTexture2D(&tex_desc, nullptr, &depth_tex));

	//==================================================================
	// create depth stencil view from texture
	D3D11_DEPTH_STENCIL_VIEW_DESC view_desc = {};

	view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MipSlice = 0;

	V(device->CreateDepthStencilView(depth_tex, &view_desc, &depth_stencil_view));

	depth_tex->Release();
}

//=================================================================================================
void Render::SetViewport()
{
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)wnd_size.x;
	viewport.Height = (float)wnd_size.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	deviceContext->RSSetViewports(1, &viewport);
}

//=================================================================================================
void Render::LogMultisampling()
{
	LocalString s = "Render: Available multisampling: ";

	/*for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels))
			&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
		{
			s += Format("x%d(%d), ", j, min(levels, levels2));
		}
	}*/

	if(s.at_back(1) == ':')
		s += "none";
	else
		s.pop(2);

	Info(s);
}

//=================================================================================================
void Render::LogAndSelectResolution()
{
	/*struct Res
	{
		int w, h, hz;

		Res(int w, int h, int hz) : w(w), h(h), hz(hz) {}
		bool operator < (const Res& r) const
		{
			if(w > r.w)
				return false;
			else if(w < r.w)
				return true;
			else if(h > r.h)
				return false;
			else if(h < r.h)
				return true;
			else if(hz > r.hz)
				return false;
			else if(hz < r.hz)
				return true;
			else
				return false;
		}
	};

	vector<Res> ress;
	LocalString str = "Render: Available display modes:";
	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
	Int2 wnd_size = app::engine->GetWindowSize();
	int best_hz = 0, best_valid_hz = 0;
	bool res_valid = false, hz_valid = false;
	for(uint i = 0; i < display_modes; ++i)
	{
		D3DDISPLAYMODE d_mode;
		V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
		if(d_mode.Width < (uint)Engine::MIN_WINDOW_SIZE.x || d_mode.Height < (uint)Engine::MIN_WINDOW_SIZE.y)
			continue;
		ress.push_back(Res(d_mode.Width, d_mode.Height, d_mode.RefreshRate));
		if(d_mode.Width == (uint)Engine::DEFAULT_WINDOW_SIZE.x && d_mode.Height == (uint)Engine::DEFAULT_WINDOW_SIZE.y)
		{
			if(d_mode.RefreshRate > (uint)best_hz)
				best_hz = d_mode.RefreshRate;
		}
		if(d_mode.Width == wnd_size.x && d_mode.Height == wnd_size.y)
		{
			res_valid = true;
			if(d_mode.RefreshRate == refresh_hz)
				hz_valid = true;
			if((int)d_mode.RefreshRate > best_valid_hz)
				best_valid_hz = d_mode.RefreshRate;
		}
	}
	std::sort(ress.begin(), ress.end());
	int cw = 0, ch = 0;
	for(vector<Res>::iterator it = ress.begin(), end = ress.end(); it != end; ++it)
	{
		Res& r = *it;
		if(r.w != cw || r.h != ch)
		{
			if(it != ress.begin())
				str += " Hz)";
			str += Format("\n\t%dx%d (%d", r.w, r.h, r.hz);
			cw = r.w;
			ch = r.h;
		}
		else
			str += Format(", %d", r.hz);
	}
	str += " Hz)";
	Info(str->c_str());

	// adjust selected resolution
	if(!res_valid)
	{
		if(wnd_size.x != 0)
		{
			Warn("Render: Resolution %dx%d is not valid, defaulting to %dx%d (%d Hz).", wnd_size.x, wnd_size.y,
				Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y, best_hz);
		}
		else
			Info("Render: Defaulting resolution to %dx%dx (%d Hz).", Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y, best_hz);
		refresh_hz = best_hz;
		app::engine->SetWindowSizeInternal(Engine::DEFAULT_WINDOW_SIZE);
	}
	else if(!hz_valid)
	{
		if(refresh_hz != 0)
			Warn("Render: Refresh rate %d Hz is not valid, defaulting to %d Hz.", refresh_hz, best_valid_hz);
		else
			Info("Render: Defaulting refresh rate to %d Hz.", best_valid_hz);
		refresh_hz = best_valid_hz;
	}*/
}

//=================================================================================================
void Render::SetDefaultRenderState()
{
	/*V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
	V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	V(device->SetRenderState(D3DRS_ALPHAREF, 200));
	V(device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL));*/

	r_alphatest = false;
	r_alphablend = false;
	r_nocull = false;
	r_nozwrite = false;
}

//=================================================================================================
void Render::CreateVertexDeclarations()
{
	/*const D3DVERTEXELEMENT9 Default[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0, 12,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,		0},
		{0, 24, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Default, &vertex_decl[VDI_DEFAULT]));

	const D3DVERTEXELEMENT9 Animated[] = {
		{0,	0,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0,	12,	D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BLENDWEIGHT,	0},
		{0,	16,	D3DDECLTYPE_UBYTE4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BLENDINDICES,	0},
		{0,	20,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,		0},
		{0,	32,	D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Animated, &vertex_decl[VDI_ANIMATED]));

	const D3DVERTEXELEMENT9 Tangents[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0, 12,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,		0},
		{0, 24, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		{0,	32,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TANGENT,		0},
		{0,	44,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BINORMAL,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Tangents, &vertex_decl[VDI_TANGENT]));

	const D3DVERTEXELEMENT9 AnimatedTangents[] = {
		{0,	0,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0,	12,	D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BLENDWEIGHT,	0},
		{0,	16,	D3DDECLTYPE_UBYTE4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BLENDINDICES,	0},
		{0,	20,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,		0},
		{0,	32,	D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		{0,	40,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TANGENT,		0},
		{0,	52,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BINORMAL,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(AnimatedTangents, &vertex_decl[VDI_ANIMATED_TANGENT]));

	const D3DVERTEXELEMENT9 Tex[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0, 12, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Tex, &vertex_decl[VDI_TEX]));

	const D3DVERTEXELEMENT9 Color[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0, 12, D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_COLOR,			0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Color, &vertex_decl[VDI_COLOR]));

	const D3DVERTEXELEMENT9 Particle[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0, 12, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		{0, 20, D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_COLOR,			0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Particle, &vertex_decl[VDI_PARTICLE]));

	const D3DVERTEXELEMENT9 Pos[] = {
		{0,	0,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Pos, &vertex_decl[VDI_POS]));*/
}

//=================================================================================================
bool Render::Reset(bool force)
{
	/*Info("Render: Reseting device.");
	BeforeReset();

	// gather params
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	GatherParams(d3dpp);

	// reset
	HRESULT hr = device->Reset(&d3dpp);
	if(FAILED(hr))
	{
		if(force || hr != D3DERR_DEVICELOST)
		{
			if(hr == D3DERR_INVALIDCALL)
				throw "Render: Device reset returned D3DERR_INVALIDCALL, not all resources was released.";
			else
				throw Format("Render: Failed to reset directx device (%d).", hr);
		}
		else
		{
			Warn("Render: Failed to reset device.");
			return false;
		}
	}

	AfterReset();*/
	return true;
}

//=================================================================================================
void Render::WaitReset()
{
	/*Info("Render: Device lost at loading. Waiting for reset.");
	BeforeReset();

	app::engine->UpdateActivity(false);

	// gather params
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	GatherParams(d3dpp);

	// wait for reset
	while(true)
	{
		app::engine->DoPseudotick(true);

		HRESULT hr = device->TestCooperativeLevel();
		if(hr == D3DERR_DEVICELOST)
		{
			Info("Render: Device lost, waiting...");
		}
		else if(hr == D3DERR_DEVICENOTRESET)
		{
			Info("Render: Device can be reseted, trying...");

			// reset
			hr = device->Reset(&d3dpp);
			if(FAILED(hr))
			{
				if(hr == D3DERR_DEVICELOST)
					Warn("Render: Can't reset, device is lost.");
				else if(hr == D3DERR_INVALIDCALL)
					throw "Render: Device reset returned D3DERR_INVALIDCALL, not all resources was released.";
				else
					throw Format("Render: Device reset returned error (%u).", hr);
			}
			else
				break;
		}
		else
			throw Format("Render: Device lost and cannot reset (%u).", hr);
		Sleep(500);
	}

	AfterReset();*/
}

//=================================================================================================
void Render::Clear(const Vec4& color)
{
	deviceContext->ClearRenderTargetView(render_target, (const float*)color);
	deviceContext->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH, 1.f, 0);
}

//=================================================================================================
void Render::Present()
{
	V(swap_chain->Present(vsync ? 1 : 0, 0));
}

//=================================================================================================
void Render::BeforeReset()
{
	/*if(res_freed)
		return;
	res_freed = true;
	V(sprite->OnLostDevice());
	for(ShaderHandler* shader : shaders)
		shader->OnReset();
	for(ManagedResource* res : managed_res)
		res->OnReset();*/
}

//=================================================================================================
void Render::AfterReset()
{
	/*SetDefaultRenderState();
	V(sprite->OnResetDevice());
	for(ShaderHandler* shader : shaders)
		shader->OnReload();
	for(ManagedResource* res : managed_res)
		res->OnReload();
	V(sprite->OnResetDevice());
	lost_device = false;
	res_freed = false;*/
}

//=================================================================================================
bool Render::CheckDisplay(const Int2& size, int& hz)
{
	/*assert(size.x >= Engine::MIN_WINDOW_SIZE.x && size.x >= Engine::MIN_WINDOW_SIZE.y);

	// check minimum resolution
	if(size.x < Engine::MIN_WINDOW_SIZE.x || size.y < Engine::MIN_WINDOW_SIZE.y)
		return false;

	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);

	if(hz == 0)
	{
		bool valid = false;

		for(uint i = 0; i < display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
			if(size.x == d_mode.Width && size.y == d_mode.Height)
			{
				valid = true;
				if(hz < (int)d_mode.RefreshRate)
					hz = d_mode.RefreshRate;
			}
		}

		return valid;
	}
	else
	{
		for(uint i = 0; i < display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
			if(size.x == d_mode.Width && size.y == d_mode.Height && hz == d_mode.RefreshRate)
				return true;
		}

		return false;
	}*/

	return false;
}

//=================================================================================================
void Render::RegisterShader(ShaderHandler* shader)
{
	assert(shader);
	shaders.push_back(shader);
	shader->OnInit();
}

//=================================================================================================
void Render::ReloadShaders()
{
	Info("Reloading shaders...");

	for(ShaderHandler* shader : shaders)
		shader->OnRelease();

	try
	{
		for(ShaderHandler* shader : shaders)
			shader->OnInit();
	}
	catch(cstring err)
	{
		app::engine->FatalError(Format("Failed to reload shaders.\n%s", err));
		return;
	}
}

//=================================================================================================
/*ID3DXEffect* Render::CompileShader(cstring name)
{
	assert(name);

	CompileShaderParams params = { name };

	// add c to extension
	LocalString str = (shader_version == 3 ? "3_" : "2_");
	str += name;
	str += 'c';
	params.cache_name = str;

	// set shader version
	D3DXMACRO macros[3] = {
		"VS_VERSION", shader_version == 3 ? "vs_3_0" : "vs_2_0",
		"PS_VERSION", shader_version == 3 ? "ps_3_0" : "ps_2_0",
		nullptr, nullptr
	};
	params.macros = macros;

	return CompileShader(params);
}

//=================================================================================================
ID3DXEffect* Render::CompileShader(CompileShaderParams& params)
{*/
	/*assert(params.name && params.cache_name);

	ID3DXBuffer* errors = nullptr;
	ID3DXEffectCompiler* compiler = nullptr;
	cstring filename = Format("%s/%s", shaders_dir.c_str(), params.name);
	cstring cache_path = Format("cache/%s", params.cache_name);
	HRESULT hr;

	const DWORD flags =
#ifdef _DEBUG
		D3DXSHADER_DEBUG | D3DXSHADER_OPTIMIZATION_LEVEL1;
#else
		D3DXSHADER_OPTIMIZATION_LEVEL3;
#endif

	// open file and get date if not from string
	FileReader file;
	if(!params.input)
	{
		if(!file.Open(filename))
			throw Format("Render: Failed to load shader '%s' (%d).", params.name, GetLastError());
		params.file_time = file.GetTime();
	}

	// check if in cache
	{
		FileReader cache_file(cache_path);
		if(cache_file && params.file_time == cache_file.GetTime())
		{
			// same last modify time, use cache
			cache_file.ReadToString(g_tmp_string);
			ID3DXEffect* effect = nullptr;
			hr = D3DXCreateEffect(device, g_tmp_string.c_str(), g_tmp_string.size(), params.macros, nullptr, flags, params.pool, &effect, &errors);
			if(FAILED(hr))
			{
				Error("Render: Failed to create effect from cache '%s' (%d).\n%s", params.cache_name, hr,
					errors ? (cstring)errors->GetBufferPointer() : "No errors information.");
				SafeRelease(errors);
				SafeRelease(effect);
			}
			else
			{
				SafeRelease(errors);
				return effect;
			}
		}
	}

	// load from file
	if(!params.input)
	{
		file.ReadToString(g_tmp_string);
		params.input = &g_tmp_string;
	}
	hr = D3DXCreateEffectCompiler(params.input->c_str(), params.input->size(), params.macros, nullptr, flags, &compiler, &errors);
	if(FAILED(hr))
	{
		cstring str;
		if(errors)
			str = (cstring)errors->GetBufferPointer();
		else
		{
			switch(hr)
			{
			case D3DXERR_INVALIDDATA:
				str = "Invalid data.";
				break;
			case D3DERR_INVALIDCALL:
				str = "Invalid call.";
				break;
			case E_OUTOFMEMORY:
				str = "Out of memory.";
				break;
			case ERROR_MOD_NOT_FOUND:
			case 0x8007007e:
				str = "Can't find module (missing d3dcompiler_43.dll?).";
				break;
			default:
				str = "Unknown error.";
				break;
			}
		}

		cstring msg = Format("Render: Failed to compile shader '%s' (%d).\n%s", params.name, hr, str);

		SafeRelease(errors);

		assert(0);
		throw msg;
	}
	SafeRelease(errors);

	// compile shader
	ID3DXBuffer* effect_buffer = nullptr;
	hr = compiler->CompileEffect(flags, &effect_buffer, &errors);
	if(FAILED(hr))
	{
		cstring msg = Format("Render: Failed to compile effect '%s' (%d).\n%s", params.name, hr,
			errors ? (cstring)errors->GetBufferPointer() : "No errors information.");

		SafeRelease(errors);
		SafeRelease(effect_buffer);
		SafeRelease(compiler);

		assert(0);
		throw msg;
	}
	SafeRelease(errors);

	// save to cache
	io::CreateDirectory("cache");
	FileWriter f(cache_path);
	if(f)
	{
		f.Write(effect_buffer->GetBufferPointer(), effect_buffer->GetBufferSize());
		f.SetTime(params.file_time);
	}
	else
		Warn("Render: Failed to save effect '%s' to cache (%d).", params.cache_name, GetLastError());

	// create effect from effect buffer
	ID3DXEffect* effect = nullptr;
	hr = D3DXCreateEffect(device, effect_buffer->GetBufferPointer(), effect_buffer->GetBufferSize(),
		params.macros, nullptr, flags, params.pool, &effect, &errors);
	if(FAILED(hr))
	{
		cstring msg = Format("Render: Failed to create effect '%s' (%d).\n%s", params.name, hr,
			errors ? (cstring)errors->GetBufferPointer() : "No errors information.");

		SafeRelease(errors);
		SafeRelease(effect_buffer);
		SafeRelease(compiler);

		assert(0);
		throw msg;
	}

	// free directx stuff
	SafeRelease(errors);
	SafeRelease(effect_buffer);
	SafeRelease(compiler);

	return effect;*/
//return nullptr;
//}

//=================================================================================================
TEX Render::CreateTexture(const Int2& size, const Color* fill)
{
	/*assert(size <= app::engine->wnd_size);
	TEX tex;
	V(device->CreateTexture(size.x, size.y, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr));
	if(fill)
	{
		TextureLock lock(tex);
		lock.Fill(*fill);
	}
	return tex;*/
	return nullptr;
}

//=================================================================================================
DynamicTexture* Render::CreateDynamicTexture(const Int2& size)
{
	/*assert(size <= app::engine->wnd_size);
	assert(size.x > 0 && size.y > 0 && IsPow2(size.x) && IsPow2(size.y));
	DynamicTexture* tex = new DynamicTexture;
	tex->size = size;
	CreateDynamicTexture(tex);
	tex->state = ResourceState::Loaded;
	managed_res.push_back(tex);
	return tex;*/
	return nullptr;
}

//=================================================================================================
void Render::CreateDynamicTexture(DynamicTexture* tex)
{
	//V(device->CreateTexture(tex->size.x, tex->size.y, 0, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex->tex, nullptr));
}

//=================================================================================================
RenderTarget* Render::CreateRenderTarget(const Int2& size)
{
	/*assert(size <= app::engine->wnd_size);
	assert(size.x > 0 && size.y > 0 && IsPow2(size.x) && IsPow2(size.y));
	RenderTarget* target = new RenderTarget;
	target->size = size;
	CreateRenderTargetTexture(target);
	managed_res.push_back(target);
	return target;*/
	return nullptr;
}

//=================================================================================================
void Render::CreateRenderTargetTexture(RenderTarget* target)
{
	/*V(device->CreateTexture(target->size.x, target->size.y, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &target->tex, nullptr));
	D3DMULTISAMPLE_TYPE type = (D3DMULTISAMPLE_TYPE)multisampling;
	if(type != D3DMULTISAMPLE_NONE)
		V(device->CreateRenderTarget(target->size.x, target->size.y, D3DFMT_A8R8G8B8, type, multisampling_quality, FALSE, &target->surf, nullptr));
	else
		target->surf = nullptr;
	target->state = ResourceState::Loaded;*/
}

//=================================================================================================
Texture* Render::CopyToTexture(RenderTarget* target)
{
	/*TEX tex = CopyToTextureRaw(target);
	if(!tex)
		return nullptr;
	Texture* t = new Texture;
	t->tex = tex;
	t->state = ResourceState::Loaded;
	return t;*/
	return nullptr;
}

//=================================================================================================
TEX Render::CopyToTextureRaw(RenderTarget* target, Int2 size)
{
	/*assert(target);

	if(size == Int2::Zero)
		size = target->GetSize();

	TEX tex;
	V(device->CreateTexture(size.x, size.y, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr));
	SURFACE surf;
	V(tex->GetSurfaceLevel(0, &surf));

	RECT rect = { 0, 0, size.x, size.y };
	HRESULT hr = D3DXLoadSurfaceFromSurface(surf, nullptr, nullptr, target->GetSurface(), nullptr, &rect, D3DX_DEFAULT, 0);

	target->FreeSurface();
	surf->Release();
	if(FAILED(hr))
	{
		tex->Release();
		WaitReset();
		return nullptr;
	}
	return tex;*/
	return nullptr;
}

//=================================================================================================
void Render::SetAlphaBlend(bool use_alphablend)
{
	if(use_alphablend != r_alphablend)
	{
		r_alphablend = use_alphablend;
		//V(device->SetRenderState(D3DRS_ALPHABLENDENABLE, r_alphablend ? TRUE : FALSE));
	}
}

//=================================================================================================
void Render::SetAlphaTest(bool use_alphatest)
{
	if(use_alphatest != r_alphatest)
	{
		r_alphatest = use_alphatest;
		//V(device->SetRenderState(D3DRS_ALPHATESTENABLE, r_alphatest ? TRUE : FALSE));
	}
}

//=================================================================================================
void Render::SetNoCulling(bool use_nocull)
{
	if(use_nocull != r_nocull)
	{
		r_nocull = use_nocull;
		//V(device->SetRenderState(D3DRS_CULLMODE, r_nocull ? D3DCULL_NONE : D3DCULL_CCW));
	}
}

//=================================================================================================
void Render::SetNoZWrite(bool use_nozwrite)
{
	if(use_nozwrite != r_nozwrite)
	{
		r_nozwrite = use_nozwrite;
		//V(device->SetRenderState(D3DRS_ZWRITEENABLE, r_nozwrite ? FALSE : TRUE));
	}
}

//=================================================================================================
void Render::SetVsync(bool new_vsync)
{
	if(new_vsync == vsync)
		return;

	vsync = new_vsync;
	if(initialized)
		Reset(true);
}

//=================================================================================================
int Render::SetMultisampling(int type, int level)
{
	/*if(type == multisampling && (level == -1 || level == multisampling_quality))
		return 1;

	if(!initialized)
	{
		multisampling = type;
		multisampling_quality = level;
		return 2;
	}

	bool fullscreen = app::engine->IsFullscreen();
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE, (D3DMULTISAMPLE_TYPE)type, &levels))
		&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE, (D3DMULTISAMPLE_TYPE)type, &levels2)))
	{
		levels = min(levels, levels2);
		if(level < 0)
			level = levels - 1;
		else if(level >= (int)levels)
			return 0;

		multisampling = type;
		multisampling_quality = level;

		Reset(true);

		return 2;
	}
	else
		return 0;*/
	return 0;
}

//=================================================================================================
void Render::GetResolutions(vector<Resolution>& v) const
{
	/*v.clear();
	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
	for(uint i = 0; i < display_modes; ++i)
	{
		D3DDISPLAYMODE d_mode;
		V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
		if(d_mode.Width >= (uint)Engine::MIN_WINDOW_SIZE.x && d_mode.Height >= (uint)Engine::MIN_WINDOW_SIZE.y)
			v.push_back({ Int2(d_mode.Width, d_mode.Height), d_mode.RefreshRate });
	}*/
}

//=================================================================================================
void Render::GetMultisamplingModes(vector<Int2>& v) const
{
	/*v.clear();
	for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels))
			&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
		{
			int level = min(levels, levels2);
			for(int i = 0; i < level; ++i)
				v.push_back(Int2(j, i));
		}
	}*/
}

//=================================================================================================
void Render::SetTarget(RenderTarget* target)
{
	/*if(target)
	{
		assert(!current_target);

		if(target->surf)
			V(device->SetRenderTarget(0, target->surf));
		else
		{
			V(target->tex->GetSurfaceLevel(0, &current_surf));
			V(device->SetRenderTarget(0, current_surf));
		}

		current_target = target;
	}
	else
	{
		assert(current_target);

		if(current_target->tmp_surf)
		{
			current_target->surf->Release();
			current_target->surf = nullptr;
			current_target->tmp_surf = false;
		}
		else
		{
			// copy to surface if using multisampling
			if(current_target->surf)
			{
				V(current_target->tex->GetSurfaceLevel(0, &current_surf));
				V(device->StretchRect(current_target->surf, nullptr, current_surf, nullptr, D3DTEXF_NONE));
			}
			current_surf->Release();
		}

		// restore old render target
		V(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &current_surf));
		V(device->SetRenderTarget(0, current_surf));
		current_surf->Release();

		current_target = nullptr;
		current_surf = nullptr;
	}*/
}

//=================================================================================================
void Render::SetTextureAddressMode(TextureAddressMode mode)
{
	//V(device->SetSamplerState(0, D3DSAMP_ADDRESSU, (D3DTEXTUREADDRESS)mode));
	//V(device->SetSamplerState(0, D3DSAMP_ADDRESSV, (D3DTEXTUREADDRESS)mode));
}

ID3D11Buffer* Render::CreateConstantBuffer(uint size)
{
	assert(size % 16 == 0);

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth = size;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	ID3D11Buffer* buffer;
	HRESULT result = device->CreateBuffer(&cbDesc, NULL, &buffer);
	if(FAILED(result))
		throw Format("Failed to create constant buffer (size:%u; code:%u).", size, result);

	return buffer;
}

ID3D11SamplerState* Render::CreateSampler(TextureAddressMode mode)
{
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	samplerDesc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	samplerDesc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* sampler;
	HRESULT result = device->CreateSamplerState(&samplerDesc, &sampler);
	if(FAILED(result))
		throw Format("Failed to create sampler state (%u).", result);

	return sampler;
}

void Render::CreateShader(cstring filename, D3D11_INPUT_ELEMENT_DESC* input, uint inputCount, ID3D11VertexShader*& vertexShader,
	ID3D11PixelShader*& pixelShader, ID3D11InputLayout*& layout, D3D_SHADER_MACRO* macro, cstring vsEntry, cstring psEntry)
{
	try
	{
		CPtr<ID3DBlob> vsBuf = CompileShader(filename, vsEntry, true, macro);
		HRESULT result = device->CreateVertexShader(vsBuf->GetBufferPointer(), vsBuf->GetBufferSize(), nullptr, &vertexShader);
		if(FAILED(result))
			throw Format("Failed to create vertex shader (%u).", result);

		CPtr<ID3DBlob> psBuf = CompileShader(filename, psEntry, false, macro);
		result = device->CreatePixelShader(psBuf->GetBufferPointer(), psBuf->GetBufferSize(), nullptr, &pixelShader);
		if(FAILED(result))
			throw Format("Failed to create pixel shader (%u).", result);

		result = device->CreateInputLayout(input, inputCount, vsBuf->GetBufferPointer(), vsBuf->GetBufferSize(), &layout);
		if(FAILED(result))
			throw Format("Failed to create input layout (%u).", result);
	}
	catch(cstring err)
	{
		throw Format("Failed to create shader '%s': %s", filename, err);
	}
}

//=================================================================================================
ID3DBlob* Render::CompileShader(cstring filename, cstring entry, bool isVertex, D3D_SHADER_MACRO* macro)
{
	assert(filename && entry);

	cstring target = isVertex ? "vs_5_0" : "ps_5_0";

	uint flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	cstring path = Format("%s/%s", shaders_dir.c_str(), filename);
	ID3DBlob* shader_blob = nullptr;
	ID3DBlob* error_blob = nullptr;
	HRESULT result = D3DCompileFromFile(ToWString(path), macro, nullptr, entry, target, flags, 0, &shader_blob, &error_blob);
	if(FAILED(result))
	{
		SafeRelease(shader_blob);
		if(error_blob)
		{
			cstring err = (cstring)error_blob->GetBufferPointer();
			cstring msg = Format("Failed to compile function %s: %s (code %u).", entry, err, result);
			error_blob->Release();
			throw msg;
		}
		else if(result == D3D11_ERROR_FILE_NOT_FOUND || result == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
			throw Format("Failed to compile function %s: file not found.", entry);
		else
			throw Format("Failed to compile function %s (code %u).", entry, result);
	}

	if(error_blob)
	{
		cstring err = (cstring)error_blob->GetBufferPointer();
		Warn("Shader '%s' warnings: %s", path, err);
		error_blob->Release();
	}

	return shader_blob;
}
