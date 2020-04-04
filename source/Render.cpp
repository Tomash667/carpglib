#include "Pch.h"
#include "Render.h"
#include "RenderTarget.h"
#include "Engine.h"
#include "ShaderHandler.h"
#include "File.h"
#include "DirectX.h"
#include <d3dcompiler.h>

Render* app::render;
static const DXGI_FORMAT DISPLAY_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

//=================================================================================================
Render::Render() : initialized(false), current_target(nullptr), /*current_surf(nullptr),*/ vsync(true),
shaders_dir("shaders"), refreshHz(0), usedAdapter(0), multisampling(0), multisampling_quality(0),

factory(nullptr), adapter(nullptr), swapChain(nullptr), device(nullptr), deviceContext(nullptr), renderTarget(nullptr), depthStencilView(nullptr),
blendStates(), depthStates(), rasterStates(), useAlphaBlend(false), depthState(DEPTH_YES), useNoCull(false), r_alphatest(false)
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

	SafeRelease(depthStencilView);
	SafeRelease(renderTarget);
	SafeRelease(swapChain);
	SafeRelease(deviceContext);
	SafeRelease(blendStates);
	SafeRelease(depthStates);
	SafeRelease(rasterStates);

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
	wndSize = app::engine->GetWindowSize();

	CreateAdapter();
	LogAndSelectResolution();
	CreateDeviceAndSwapChain();
	CreateSizeDependentResources();
	CreateBlendStates();
	CreateDepthStates();
	CreateRasterStates();

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// check shaders version
	/*D3DCAPS9 caps;
	hr = d3d->GetDeviceCaps(usedAdapter, D3DDEVTYPE_HAL, &caps);
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
	hr = d3d->CheckDeviceType(usedAdapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, BACKBUFFER_FORMAT, fullscreen ? FALSE : TRUE);
	if(FAILED(hr))
		throw Format("Render: Unsupported backbuffer type %s for display %s! (%d)", STRING(BACKBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDeviceFormat(usedAdapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Render: Unsupported depth buffer type %s for display %s! (%d)", STRING(ZBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDepthStencilMatch(usedAdapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DFMT_A8R8G8B8, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Render: Unsupported render target D3DFMT_A8R8G8B8 with display %s and depth buffer %s! (%d)",
		STRING(DISPLAY_FORMAT), STRING(BACKBUFFER_FORMAT), hr);

	// check multisampling
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels))
		&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE,
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
		hr = d3d->CreateDevice(usedAdapter, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, sel_mode, &d3dpp, &device);

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
		if(usedAdapter == i)
			adapter = tmp_adapter;
		else
			tmp_adapter->Release();
	}

	// fallback to first adapter
	if(!adapter)
	{
		Warn("Render: Invalid adapter %d, defaulting to 0.", usedAdapter);
		usedAdapter = 0;
		V(factory->EnumAdapters(0, &adapter));
	}
}

//=================================================================================================
void Render::CreateDeviceAndSwapChain()
{
	DXGI_SWAP_CHAIN_DESC swap_desc = {};
	swap_desc.BufferCount = 1;
	swap_desc.BufferDesc.Width = wndSize.x;
	swap_desc.BufferDesc.Height = wndSize.y;
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
		D3D11_SDK_VERSION, &swap_desc, &swapChain, &device, &feature_level, &deviceContext);
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
	deviceContext->OMSetRenderTargets(1, &renderTarget, depthStencilView);
	SetViewport();
}

//=================================================================================================
void Render::CreateRenderTarget()
{
	HRESULT result;
	ID3D11Texture2D* back_buffer;
	result = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
	if(FAILED(result))
		throw Format("Failed to get back buffer (%u).", result);

	// Create the render target view with the back buffer pointer.
	result = device->CreateRenderTargetView(back_buffer, NULL, &renderTarget);
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

	tex_desc.Width = wndSize.x;
	tex_desc.Height = wndSize.y;
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

	V(device->CreateDepthStencilView(depth_tex, &view_desc, &depthStencilView));

	depth_tex->Release();
}

//=================================================================================================
void Render::SetViewport()
{
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)wndSize.x;
	viewport.Height = (float)wndSize.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	deviceContext->RSSetViewports(1, &viewport);
}

//=================================================================================================
void Render::CreateBlendStates()
{
	// get disabled blend state
	deviceContext->OMGetBlendState(&blendStates[0], nullptr, nullptr);

	// create enabled blend state
	D3D11_BLEND_DESC desc = {};
	desc.RenderTarget[0].BlendEnable = true;
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	V(device->CreateBlendState(&desc, &blendStates[1]));
}

//=================================================================================================
void Render::CreateDepthStates()
{
	// create depth stencil state
	D3D11_DEPTH_STENCIL_DESC desc = {};
	desc.DepthEnable = true;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	desc.StencilEnable = false;
	desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	desc.BackFace = desc.FrontFace;

	V(device->CreateDepthStencilState(&desc, &depthStates[DEPTH_YES]));
	deviceContext->OMSetDepthStencilState(depthStates[DEPTH_YES], 1);

	// create depth stencil state with disabled depth test
	desc.DepthEnable = false;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	V(device->CreateDepthStencilState(&desc, &depthStates[DEPTH_NO]));

	// create readonly depth stencil state
	desc.DepthEnable = true;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	V(device->CreateDepthStencilState(&desc, &depthStates[DEPTH_READONLY]));
}

//=================================================================================================
void Render::CreateRasterStates()
{
	FIXME;
}

//=================================================================================================
void Render::LogMultisampling()
{
	LocalString s = "Render: Available multisampling: ";

	/*for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels))
			&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
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
	// enum display modes
	IDXGIOutput* output;
	V(adapter->EnumOutputs(0, &output));

	uint count;
	V(output->GetDisplayModeList(DISPLAY_FORMAT, 0, &count, nullptr));

	Buf buf;
	DXGI_MODE_DESC* modes = buf.Get<DXGI_MODE_DESC>(sizeof(DXGI_MODE_DESC) * count);
	V(output->GetDisplayModeList(DISPLAY_FORMAT, 0, &count, modes));

	output->Release();

	// list all resolutions
	Int2 wndSize = app::engine->GetWindowSize();
	int bestDefHz = 0, bestHz = 0;
	bool resValid = false, hzValid = false;
	for(uint i = 0; i < count; ++i)
	{
		DXGI_MODE_DESC& mode = modes[i];
		if(mode.Width < (uint)Engine::MIN_WINDOW_SIZE.x || mode.Height < (uint)Engine::MIN_WINDOW_SIZE.y)
			continue;
		uint hz = mode.RefreshRate.Numerator / mode.RefreshRate.Denominator;
		resolutions.push_back({ Int2(mode.Width, mode.Height), hz });
		if(mode.Width == (uint)Engine::DEFAULT_WINDOW_SIZE.x && mode.Height == (uint)Engine::DEFAULT_WINDOW_SIZE.y)
		{
			if(hz > (uint)bestDefHz)
				bestDefHz = hz;
		}
		if(mode.Width == wndSize.x && mode.Height == wndSize.y)
		{
			resValid = true;
			if(hz == refreshHz)
				hzValid = true;
			if((int)hz > bestHz)
				bestHz = hz;
		}
	}
	std::sort(resolutions.begin(), resolutions.end());

	// pretty print
	int cw = 0, ch = 0;
	LocalString str = "Render: Available display modes:";
	for(vector<Resolution>::iterator it = resolutions.begin(), end = resolutions.end(); it != end; ++it)
	{
		Resolution& r = *it;
		if(r.size.x != cw || r.size.y != ch)
		{
			if(it != resolutions.begin())
				str += " Hz)";
			str += Format("\n\t%dx%d (%u", r.size.x, r.size.y, r.hz);
			cw = r.size.x;
			ch = r.size.y;
		}
		else
			str += Format(", %d", r.hz);
	}
	str += " Hz)";
	Info(str->c_str());

	// adjust selected resolution
	if(!resValid)
	{
		if(wndSize.x != 0)
		{
			Warn("Render: Resolution %dx%d is not valid, defaulting to %dx%d (%d Hz).", wndSize.x, wndSize.y,
				Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y, bestDefHz);
		}
		else
			Info("Render: Defaulting resolution to %dx%dx (%d Hz).", Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y, bestDefHz);
		refreshHz = bestDefHz;
		app::engine->SetWindowSizeInternal(Engine::DEFAULT_WINDOW_SIZE);
	}
	else if(!hzValid)
	{
		if(refreshHz != 0)
			Warn("Render: Refresh rate %d Hz is not valid, defaulting to %d Hz.", refreshHz, bestHz);
		else
			Info("Render: Defaulting refresh rate to %d Hz.", bestHz);
		refreshHz = bestHz;
	}
}

//=================================================================================================
void Render::Clear(const Vec4& color)
{
	deviceContext->ClearRenderTargetView(renderTarget, (const float*)color);
	deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.f, 0);
}

//=================================================================================================
void Render::Present()
{
	V(swapChain->Present(vsync ? 1 : 0, 0));
}

//=================================================================================================
bool Render::CheckDisplay(const Int2& size, uint& hz)
{
	/*assert(size.x >= Engine::MIN_WINDOW_SIZE.x && size.x >= Engine::MIN_WINDOW_SIZE.y);

	// check minimum resolution
	if(size.x < Engine::MIN_WINDOW_SIZE.x || size.y < Engine::MIN_WINDOW_SIZE.y)
		return false;

	uint display_modes = d3d->GetAdapterModeCount(usedAdapter, DISPLAY_FORMAT);

	if(hz == 0)
	{
		bool valid = false;

		for(uint i = 0; i < display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(usedAdapter, DISPLAY_FORMAT, i, &d_mode));
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
			V(d3d->EnumAdapterModes(usedAdapter, DISPLAY_FORMAT, i, &d_mode));
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
TEX Render::CreateRawTexture(const Int2& size, const Color* fill)
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	ID3D11Texture2D* tex;
	if(fill)
	{
		desc.Usage = D3D11_USAGE_IMMUTABLE;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = fill;
		initData.SysMemPitch = sizeof(Color) * size.x;
		V(device->CreateTexture2D(&desc, &initData, &tex));
	}
	else
	{
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		V(device->CreateTexture2D(&desc, nullptr, &tex));
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* view;
	V(device->CreateShaderResourceView(tex, &viewDesc, &view));
	tex->Release();

	return view;
}

//=================================================================================================
Texture* Render::CreateTexture(const Int2& size)
{
	Texture* tex = new Texture;
	tex->tex = CreateRawTexture(size);
	tex->state = ResourceState::Loaded;
	return tex;
}

//=================================================================================================
RenderTarget* Render::CreateRenderTarget(const Int2& size)
{
	assert(size <= wndSize);
	assert(size.x > 0 && size.y > 0 && IsPow2(size.x) && IsPow2(size.y));
	RenderTarget* target = new RenderTarget;
	target->size = size;
	target->tex = CreateRawTexture(size);
	return target;
}

//=================================================================================================
Texture* Render::CopyToTexture(RenderTarget* target)
{
	assert(target);

	Texture* t = new Texture;
	t->tex = CopyToTextureRaw(target);
	t->state = ResourceState::Loaded;
	return t;
}

//=================================================================================================
TEX Render::CopyToTextureRaw(RenderTarget* target)
{
	assert(target);

	// get render target texture data
	ID3D11Texture2D* res;
	target->tex->GetResource(reinterpret_cast<ID3D11Resource**>(&res));

	D3D11_TEXTURE2D_DESC texDesc;
	res->GetDesc(&texDesc);

	// create new texture
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = texDesc.Width;
	desc.Height = texDesc.Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;

	ID3D11Texture2D* tex;
	V(device->CreateTexture2D(&desc, nullptr, &tex));

	// copy texture
	deviceContext->CopyResource(tex, res);

	// create view
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* view;
	V(device->CreateShaderResourceView(tex, &viewDesc, &view));

	tex->Release();
	res->Release();
	return view;
}

//=================================================================================================
void Render::SetAlphaBlend(bool useAlphaBlend)
{
	if(useAlphaBlend != this->useAlphaBlend)
	{
		this->useAlphaBlend = useAlphaBlend;
		deviceContext->OMSetBlendState(blendStates[useAlphaBlend ? 1 : 0], nullptr, 0xFFFFFFFF);
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
void Render::SetDepthState(DepthState depthState)
{
	assert(depthState >= 0 && depthState < DEPTH_MAX);
	if(this->depthState != depthState)
	{
		this->depthState = depthState;
		deviceContext->OMSetDepthStencilState(depthStates[depthState], 0);
	}
}

//=================================================================================================
void Render::SetNoCulling(bool use_nocull)
{
	//if(use_nocull != r_nocull)
	{
		//r_nocull = use_nocull;
		//V(device->SetRenderState(D3DRS_CULLMODE, r_nocull ? D3DCULL_NONE : D3DCULL_CCW));
	}
	FIXME;
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
void Render::GetMultisamplingModes(vector<Int2>& v) const
{
	/*v.clear();
	for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels))
			&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
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
	size = alignto(size, 16);

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
