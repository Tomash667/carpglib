#include "Pch.h"
#include "Render.h"

#include "DirectX.h"
#include "Engine.h"
#include "File.h"
#include "RenderTarget.h"
#include "ScreenGrab.h"
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

#include <d3dcompiler.h>

Render* app::render;
static const DXGI_FORMAT DISPLAY_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

//=================================================================================================
Render::Render() : initialized(false), vsync(true), shadersDir("shaders"), usedAdapter(0), multisampling(0), multisamplingQuality(0),
factory(nullptr), adapter(nullptr), swapChain(nullptr), device(nullptr), deviceContext(nullptr), renderTargetView(nullptr), depthStencilView(nullptr),
blendStates(), depthStates(), rasterStates(), blendState(BLEND_NO), depthState(DEPTH_YES), rasterState(RASTER_NORMAL), currentTarget(nullptr),
forceFeatureLevel(0), defaultSampler(nullptr)
{
}

//=================================================================================================
Render::~Render()
{
	for(ShaderHandler* shader : shaders)
	{
		shader->OnRelease();
		delete shader;
	}
	DeleteElements(renderTargets);

	SafeRelease(depthStencilView);
	SafeRelease(renderTargetView);
	SafeRelease(swapChain);
	SafeRelease(deviceContext);
	SafeRelease(blendStates);
	SafeRelease(depthStates);
	SafeRelease(rasterStates);
	SafeRelease(defaultSampler);

	if(device)
	{
		if(IsDebug())
		{
			// write to output directx leaks
			ID3D11Debug* debug;
			device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug);
			if(debug)
			{
				debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
				debug->Release();
			}
		}
		device->Release();
	}

	SafeRelease(adapter);
	SafeRelease(factory);
}

//=================================================================================================
void Render::Init()
{
	wndSize = app::engine->GetClientSize();

	io::CreateDirectory("cache");

	CreateAdapter();
	CreateDevice();
	LogAndSelectResolution();
	LogAndSelectMultisampling();
	CreateSwapChain();
	CreateSizeDependentResources();
	CreateBlendStates();
	CreateDepthStates();
	CreateRasterStates();

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	defaultSampler = CreateSampler();

	initialized = true;
	Info("Render: Initialization finished.");
}

//=================================================================================================
void Render::OnChangeResolution()
{
	if(!swapChain)
		return;

	wndSize = app::engine->GetClientSize();

	SafeRelease(rasterStates);
	SafeRelease(depthStencilView);
	SafeRelease(renderTargetView);
	SafeRelease(swapChain);

	CreateSwapChain();
	CreateRenderTargetView();
	depthStencilView = CreateDepthStencilView(wndSize);
	deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
	SetViewport(wndSize);
	CreateRasterStates();
	for(RenderTarget* target : renderTargets)
		RecreateRenderTarget(target);
}

//=================================================================================================
void Render::CreateAdapter()
{
	// create dxgi factory
	V(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory));

	// enumerate adapters
	IDXGIAdapter* tmpAdapter;
	for(int i = 0; factory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC desc;
		V(tmpAdapter->GetDesc(&desc));
		Info("Render: Adapter %d: %s", i, ToString(desc.Description));
		if(usedAdapter == i)
			adapter = tmpAdapter;
		else
			tmpAdapter->Release();
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
cstring GetFeatureLevelString(int value)
{
	return Format("%d.%d", (value & 0xF000) >> 12, (value & 0xF00) >> 8);
}

//=================================================================================================
void Render::CreateDevice()
{
	int flags = IsDebug() ? D3D11_CREATE_DEVICE_DEBUG : 0;
	D3D_FEATURE_LEVEL featureLevel;

	if(forceFeatureLevel != 0)
	{
		D3D_FEATURE_LEVEL featureLevels[] = { (D3D_FEATURE_LEVEL)forceFeatureLevel };
		HRESULT result = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, featureLevels, countof(featureLevels),
			D3D11_SDK_VERSION, &device, &featureLevel, &deviceContext);
		if(result == DXGI_ERROR_SDK_COMPONENT_MISSING && flags != 0)
		{
			flags = 0;
			result = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, featureLevels, countof(featureLevels),
				D3D11_SDK_VERSION, &device, &featureLevel, &deviceContext);
			if(SUCCEEDED(result))
				Warn("Render: Failed to create device with debug mode.");
		}
		if(FAILED(result))
		{
			Warn("Render: Failed to create device in feature level %s (%u). Retrying...", GetFeatureLevelString(forceFeatureLevel), result);
			forceFeatureLevel = 0;
		}
	}

	if(forceFeatureLevel == 0)
	{
		D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
		HRESULT result = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, featureLevels, countof(featureLevels),
			D3D11_SDK_VERSION, &device, &featureLevel, &deviceContext);
		if(result == DXGI_ERROR_SDK_COMPONENT_MISSING && flags != 0)
		{
			flags = 0;
			result = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, featureLevels, countof(featureLevels),
				D3D11_SDK_VERSION, &device, &featureLevel, &deviceContext);
			if(SUCCEEDED(result))
				Warn("Render: Failed to create device with debug mode.");
		}
		if(FAILED(result))
			throw Format("Render: Failed to create device (%u).", result);
	}

	Info("Render: Device created with %s feature level.", GetFeatureLevelString(featureLevel));
	useV4Shaders = (featureLevel != D3D_FEATURE_LEVEL_11_0);
}

//=================================================================================================
void Render::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC desc = {};
	desc.BufferCount = 1;
	desc.BufferDesc.Width = wndSize.x;
	desc.BufferDesc.Height = wndSize.y;
	desc.BufferDesc.Format = DISPLAY_FORMAT;
	desc.BufferDesc.RefreshRate.Numerator = 0;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = app::engine->GetWindowHandle();
	desc.SampleDesc.Count = max(multisampling, 1);
	desc.SampleDesc.Quality = multisamplingQuality;
	desc.Windowed = true;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT result = factory->CreateSwapChain(device, &desc, &swapChain);
	if(FAILED(result))
		throw Format("Failed to create swap chain (%u).", result);

	// disable builtin alt+enter
	V(factory->MakeWindowAssociation(desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES));
}

//=================================================================================================
void Render::CreateSizeDependentResources()
{
	CreateRenderTargetView();
	depthStencilView = CreateDepthStencilView(wndSize);
	deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
	SetViewport(wndSize);
}

//=================================================================================================
void Render::CreateRenderTargetView()
{
	ID3D11Texture2D* backBuffer;
	V(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

	V(device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView));

	backBuffer->Release();
}

//=================================================================================================
ID3D11DepthStencilView* Render::CreateDepthStencilView(const Int2& size, bool useMs)
{
	useMs = useMs && multisampling;

	// create depth texture
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	if(useMs)
	{
		desc.SampleDesc.Count = max(multisampling, 1);
		desc.SampleDesc.Quality = multisamplingQuality;
	}
	else
		desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D* tex;
	V(device->CreateTexture2D(&desc, nullptr, &tex));

	// create depth stencil view from texture
	D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	viewDesc.ViewDimension = useMs ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;

	ID3D11DepthStencilView* view;
	V(device->CreateDepthStencilView(tex, &viewDesc, &view));

	tex->Release();
	return view;
}

//=================================================================================================
void Render::SetViewport(const Int2& size)
{
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)size.x;
	viewport.Height = (float)size.y;
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
	deviceContext->OMGetBlendState(&blendStates[BLEND_NO], nullptr, nullptr);

	// create additive blend state
	D3D11_BLEND_DESC desc = {};
	desc.RenderTarget[0].BlendEnable = true;
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	V(device->CreateBlendState(&desc, &blendStates[BLEND_ADD]));

	// create additive one blend state
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	V(device->CreateBlendState(&desc, &blendStates[BLEND_ADD_ONE]));

	// create additive one to one blend state
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	V(device->CreateBlendState(&desc, &blendStates[BLEND_ADD_ONE2]));

	// create reverse subtract blend state
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	V(device->CreateBlendState(&desc, &blendStates[BLEND_REV_SUBTRACT]));
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
	V(device->CreateDepthStencilState(&desc, &depthStates[DEPTH_READ]));
}

//=================================================================================================
void Render::CreateRasterStates()
{
	// create normal raster state
	D3D11_RASTERIZER_DESC desc = {};
	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_BACK;
	desc.DepthClipEnable = true;
	desc.MultisampleEnable = multisampling > 0;
	desc.AntialiasedLineEnable = multisampling > 0;
	V(device->CreateRasterizerState(&desc, &rasterStates[RASTER_NORMAL]));

	// create disabled culling raster state
	desc.CullMode = D3D11_CULL_NONE;
	V(device->CreateRasterizerState(&desc, &rasterStates[RASTER_NO_CULLING]));

	// create wireframe raster state
	desc.FillMode = D3D11_FILL_WIREFRAME;
	V(device->CreateRasterizerState(&desc, &rasterStates[RASTER_WIREFRAME]));
}

//=================================================================================================
void Render::LogAndSelectResolution()
{
	// enum display modes
	IDXGIOutput* output;
	V(adapter->EnumOutputs(0, &output));

	uint count;
	HRESULT result = output->GetDisplayModeList(DISPLAY_FORMAT, 0, &count, nullptr);
	if(result == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
	{
		// temporary workaround, resolutions are only required for fullscreen exclusive
		resolutions.push_back({ Engine::DEFAULT_WINDOW_SIZE });
		return;
	}
	V(result);

	Buf buf;
	DXGI_MODE_DESC* modes = buf.Get<DXGI_MODE_DESC>(sizeof(DXGI_MODE_DESC) * count);
	V(output->GetDisplayModeList(DISPLAY_FORMAT, 0, &count, modes));

	output->Release();

	// list all resolutions
	Int2 wndSize = app::engine->GetWindowSize();
	Int2 prevRes = Int2::Zero;
	bool resValid = false;
	for(uint i = 0; i < count; ++i)
	{
		DXGI_MODE_DESC& mode = modes[i];
		if(mode.Width < (uint)Engine::MIN_WINDOW_SIZE.x || mode.Height < (uint)Engine::MIN_WINDOW_SIZE.y
			|| (prevRes.x == mode.Width && prevRes.y == mode.Height))
			continue;
		prevRes = Int2(mode.Width, mode.Height);
		resolutions.push_back({ prevRes });
		if(wndSize == prevRes)
			resValid = true;
	}
	std::sort(resolutions.begin(), resolutions.end());

	// pretty print
	LocalString str = "Render: Available display modes: ";
	for(vector<Resolution>::iterator it = resolutions.begin(), end = resolutions.end(); it != end; ++it)
		str += Format("%dx%d, ", it->size.x, it->size.y);
	str.pop(2);
	str += ".";
	Info(str->c_str());

	// adjust selected resolution
	if(!resValid)
	{
		if(wndSize.x != 0)
		{
			Warn("Render: Resolution %dx%d is not valid, defaulting to %dx%d.", wndSize.x, wndSize.y,
				Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y);
		}
		else
			Info("Render: Defaulting resolution to %dx%dx.", Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y);
		app::engine->SetWindowSizeInternal(Engine::DEFAULT_WINDOW_SIZE);
	}
}

//=================================================================================================
void Render::LogAndSelectMultisampling()
{
	LocalString s = "Render: Available multisampling: ";

	bool ok = false;
	for(uint i = 2; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; i *= 2)
	{
		uint levels;
		V(device->CheckMultisampleQualityLevels(DISPLAY_FORMAT, i, &levels));
		if(levels == 0)
			continue;
		multisampleLevels.push_back(Int2(i, levels));
		s += Format("x%u(%u), ", i, levels);
		if(i == multisampling)
		{
			ok = true;
			if((uint)multisamplingQuality >= levels)
			{
				Warn("Render: Unavailable multisampling quality, changed to 0.");
				multisamplingQuality = 0;
			}
		}
	}

	if(!ok)
	{
		if(multisampling != 0)
			Warn("Render: Unavailable multisampling mode, disabling.");
		multisampling = 0;
		multisamplingQuality = 0;
	}

	if(s.at_back(1) == ':')
		s += "none";
	else
		s.pop(2);

	Info(s);
}

//=================================================================================================
void Render::Clear(const Vec4& color)
{
	ID3D11RenderTargetView* renderTargetView;
	ID3D11DepthStencilView* depthStencilView;
	deviceContext->OMGetRenderTargets(1, &renderTargetView, &depthStencilView);
	deviceContext->ClearRenderTargetView(renderTargetView, (const float*)color);
	deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.f, 0);
	renderTargetView->Release();
	depthStencilView->Release();
}

//=================================================================================================
void Render::Present()
{
	assert(!currentTarget);
	V(swapChain->Present(vsync ? 1 : 0, 0));
}

//=================================================================================================
bool Render::CheckDisplay(const Int2& size) const
{
	// check minimum resolution
	if(size.x < Engine::MIN_WINDOW_SIZE.x || size.y < Engine::MIN_WINDOW_SIZE.y)
		return false;

	for(const Resolution& resolution : resolutions)
	{
		if(resolution.size == size)
			return true;
	}

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
DynamicTexture* Render::CreateDynamicTexture(const Int2& size)
{
	DynamicTexture* tex = new DynamicTexture;
	tex->state = ResourceState::Loaded;

	// create texture
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	V(device->CreateTexture2D(&desc, nullptr, &tex->texResource));

	// create staging texture
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	V(device->CreateTexture2D(&desc, nullptr, &tex->texStaging));

	// create srv
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;
	V(device->CreateShaderResourceView(tex->texResource, &viewDesc, &tex->tex));

	return tex;
}

//=================================================================================================
TEX Render::CreateImmutableTexture(const Int2& size, const Color* fill)
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = fill;
	initData.SysMemPitch = sizeof(Color) * size.x;

	ID3D11Texture2D* tex;
	V(device->CreateTexture2D(&desc, &initData, &tex));

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
RenderTarget* Render::CreateRenderTarget(const Int2& size, int flags)
{
	assert(size <= wndSize);
	assert((size.x > 0 && size.y > 0 && IsPow2(size.x) && IsPow2(size.y)) || size == Int2::Zero);
	RenderTarget* target = new RenderTarget;
	if(size == Int2::Zero)
		flags |= RenderTarget::F_USE_WINDOW_SIZE;
	target->size = size;
	target->flags = flags;
	target->state = ResourceState::Loaded;
	CreateRenderTargetInternal(target);
	renderTargets.push_back(target);
	return target;
}

//=================================================================================================
void Render::RecreateRenderTarget(RenderTarget* target)
{
	if(target->tex)
		target->tex->Release();
	if(target->depthStencilView)
		target->depthStencilView->Release();
	target->renderTargetView->Release();
	CreateRenderTargetInternal(target);
}

//=================================================================================================
void Render::CreateRenderTargetInternal(RenderTarget* target)
{
	bool ms = multisampling && !IsSet(target->flags, RenderTarget::F_NO_MS);

	if(IsSet(target->flags, RenderTarget::F_USE_WINDOW_SIZE))
		target->size = wndSize;

	// create texture
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = target->size.x;
	desc.Height = target->size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	if(ms)
	{
		desc.SampleDesc.Count = multisampling;
		desc.SampleDesc.Quality = multisamplingQuality;
	}
	else
		desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	ID3D11Texture2D* texResource;
	V(device->CreateTexture2D(&desc, nullptr, &texResource));

	// create render target view
	V(device->CreateRenderTargetView(texResource, nullptr, &target->renderTargetView));

	if(IsSet(target->flags, RenderTarget::F_NO_DRAW))
		target->tex = nullptr;
	else
	{
		// create non multisampled texture
		// (multisampled texture can't be used to draw so it will be first copied to normal texture)
		if(ms)
		{
			texResource->Release();
			ms = false;

			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.MiscFlags = 0;

			V(device->CreateTexture2D(&desc, nullptr, &texResource));
		}

		// create srv
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		viewDesc.ViewDimension = ms ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipLevels = 1;
		V(device->CreateShaderResourceView(texResource, &viewDesc, &target->tex));
	}
	texResource->Release();

	// create depth stencil view
	if(IsSet(target->flags, RenderTarget::F_NO_DEPTH))
		target->depthStencilView = nullptr;
	else
		target->depthStencilView = CreateDepthStencilView(target->size, !IsSet(target->flags, RenderTarget::F_NO_MS));
}

//=================================================================================================
Texture* Render::CopyToTexture(RenderTarget* target)
{
	assert(target);

	Texture* tex = new Texture;
	tex->tex = CopyToTextureRaw(target);
	tex->state = ResourceState::Loaded;
	return tex;
}

//=================================================================================================
TEX Render::CopyToTextureRaw(RenderTarget* target)
{
	assert(target);

	// get render target texture data
	ID3D11Texture2D* res;
	if(target->tex)
		target->tex->GetResource(reinterpret_cast<ID3D11Resource**>(&res));
	else
		target->GetRenderTargetView()->GetResource(reinterpret_cast<ID3D11Resource**>(&res));

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
	if(texDesc.SampleDesc.Count > 1)
		deviceContext->ResolveSubresource(tex, 0, res, 0, desc.Format);
	else
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
void Render::SetBlendState(BlendState blendState)
{
	assert(blendState >= 0 && blendState < BLEND_MAX);
	if(this->blendState != blendState)
	{
		this->blendState = blendState;
		deviceContext->OMSetBlendState(blendStates[blendState], nullptr, 0xFFFFFFFF);
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
void Render::SetRasterState(RasterState rasterState)
{
	assert(rasterState >= 0 && rasterState < RASTER_MAX);
	if(this->rasterState != rasterState)
	{
		this->rasterState = rasterState;
		deviceContext->RSSetState(rasterStates[rasterState]);
	}
}

//=================================================================================================
int Render::SetMultisampling(int type, int level)
{
	if(type == multisampling && (level == -1 || level == multisamplingQuality))
		return 1;

	if(!initialized)
	{
		multisampling = type;
		multisamplingQuality = level;
		return 2;
	}

	if(type != 0)
	{
		bool ok = false;
		for(Int2& ms : multisampleLevels)
		{
			if(ms.x == type && ms.y >= level)
			{
				ok = true;
				if(level == -1)
					level = ms.y;
				break;
			}
		}
		if(!ok)
			return 0;
	}
	else
		level = 0;

	multisampling = type;
	multisamplingQuality = level;

	OnChangeResolution();

	return 2;
}

//=================================================================================================
void Render::SetRenderTarget(RenderTarget* target)
{
	if(target)
	{
		deviceContext->OMSetRenderTargets(1, &target->renderTargetView, target->depthStencilView);
		SetViewport(target->size);

		currentTarget = target;
	}
	else
	{
		if(currentTarget && multisampling && !IsSet(currentTarget->flags, RenderTarget::F_NO_DRAW))
		{
			// copy ms texture to normal
			ID3D11Texture2D* texMS, *texNormal;
			currentTarget->renderTargetView->GetResource(reinterpret_cast<ID3D11Resource**>(&texMS));
			currentTarget->tex->GetResource(reinterpret_cast<ID3D11Resource**>(&texNormal));
			deviceContext->ResolveSubresource(texNormal, 0, texMS, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
			texMS->Release();
			texNormal->Release();
		}

		deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
		SetViewport(wndSize);

		currentTarget = nullptr;
	}
}

//=================================================================================================
ID3D11Buffer* Render::CreateConstantBuffer(uint size, cstring name)
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
	HRESULT result = device->CreateBuffer(&cbDesc, nullptr, &buffer);
	if(FAILED(result))
		throw Format("Failed to create constant buffer (size:%u; code:%u).", size, result);

	SetDebugName(buffer, name);
	return buffer;
}

//=================================================================================================
ID3D11SamplerState* Render::CreateSampler(TextureAddressMode mode, bool disableMipmap)
{
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	samplerDesc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	samplerDesc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MaxLOD = disableMipmap ? 0 : FLT_MAX;

	ID3D11SamplerState* sampler;
	HRESULT result = device->CreateSamplerState(&samplerDesc, &sampler);
	if(FAILED(result))
		throw Format("Failed to create sampler state (%u).", result);

	return sampler;
}

//=================================================================================================
void Render::CreateShader(ShaderParams& params)
{
	try
	{
		if(params.vertexShader)
		{
			CPtr<ID3DBlob> vsBlob = CompileShader(params, true);
			HRESULT result = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, params.vertexShader);
			if(FAILED(result))
				throw Format("Failed to create vertex shader (%u).", result);
			SetDebugName(*params.vertexShader, Format("%s/%s", params.name, params.vsEntry));

			if(params.layout)
			{
				VertexDeclaration& vertDecl = VertexDeclaration::decl[(int)params.decl];
				result = device->CreateInputLayout(vertDecl.desc, vertDecl.count, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), params.layout);
				if(FAILED(result))
					throw Format("Failed to create input layout (%u).", result);
				SetDebugName(*params.layout, Format("%s/layout", params.name));
			}

			if(params.vsBlob)
				*params.vsBlob = vsBlob.Pin();
		}

		if(params.pixelShader)
		{
			CPtr<ID3DBlob> psBlob = CompileShader(params, false);
			HRESULT result = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, params.pixelShader);
			if(FAILED(result))
				throw Format("Failed to create pixel shader (%u).", result);
			SetDebugName(*params.pixelShader, Format("%s/%s", params.name, params.psEntry));
		}
	}
	catch(cstring err)
	{
		throw Format("Failed to create shader '%s': %s", params.name, err);
	}
}

//=================================================================================================
ID3DBlob* Render::CompileShader(ShaderParams& params, bool isVertex)
{
	cstring entry, target;
	if(isVertex)
	{
		entry = params.vsEntry;
		target = useV4Shaders ? "vs_4_0" : "vs_5_0";
	}
	else
	{
		entry = params.psEntry;
		target = useV4Shaders ? "ps_4_0" : "ps_5_0";
	}

	const uint flags = D3DCOMPILE_ENABLE_STRICTNESS | (IsDebug() ? D3DCOMPILE_DEBUG : 0);
	cstring path = Format("%s/%s.hlsl", shadersDir.c_str(), params.name);
	FileTime fileTime = io::GetFileTime(path);

	// try to load from cache
	ID3DBlob* shaderBlob = nullptr;
	HRESULT result;
	cstring cache = Format("cache/%s_%s.%s", params.cacheName ? params.cacheName : params.name, entry, target);
	FileTime cacheTime = io::GetFileTime(cache);
	if(cacheTime >= fileTime)
	{
		result = D3DReadFileToBlob(ToWString(cache), &shaderBlob);
		if(SUCCEEDED(result))
			return shaderBlob;
	}

	// compile from file or string
	ID3DBlob* errorBlob = nullptr;
	if(params.code)
		result = D3DCompile(params.code->data(), params.code->length(), params.name, params.macro, nullptr, entry, target, flags, 0, &shaderBlob, &errorBlob);
	else
		result = D3DCompileFromFile(ToWString(path), params.macro, nullptr, entry, target, flags, 0, &shaderBlob, &errorBlob);
	if(FAILED(result))
	{
		SafeRelease(shaderBlob);
		if(errorBlob)
		{
			cstring err = (cstring)errorBlob->GetBufferPointer();
			cstring msg = Format("Failed to compile function %s: %s (code %u).", entry, err, result);
			errorBlob->Release();
			throw msg;
		}
		else if(result == D3D11_ERROR_FILE_NOT_FOUND || result == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
			throw Format("Failed to compile function %s: file not found.", entry);
		else
			throw Format("Failed to compile function %s (code %u).", entry, result);
	}

	// write warning
	if(errorBlob)
	{
		cstring err = (cstring)errorBlob->GetBufferPointer();
		Warn("Shader '%s' warnings: %s", params.name, err);
		errorBlob->Release();
	}

	// save to cache
	V(D3DWriteBlobToFile(shaderBlob, ToWString(cache), true));

	return shaderBlob;
}

//=================================================================================================
ID3D11InputLayout* Render::CreateInputLayout(VertexDeclarationId decl, ID3DBlob* vsBlob, cstring name)
{
	ID3D11InputLayout* layout;
	VertexDeclaration& vertDecl = VertexDeclaration::decl[(int)decl];

	HRESULT result = device->CreateInputLayout(vertDecl.desc, vertDecl.count, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &layout);
	if(FAILED(result))
		throw Format("Failed to create input layout (%u).", result);

	SetDebugName(layout, name);
	return layout;
}

//=================================================================================================
void Render::SaveToFile(TEX tex, cstring path, ImageFormat format)
{
	ID3D11Texture2D* res;
	tex->GetResource(reinterpret_cast<ID3D11Resource**>(&res));

	V(SaveWICTextureToFile(deviceContext, res, ImageFormatMethods::ToGuid(format), ToWString(path)));

	res->Release();
}

//=================================================================================================
uint Render::SaveToFile(TEX tex, FileWriter& file, ImageFormat format)
{
	ID3D11Texture2D* res;
	tex->GetResource(reinterpret_cast<ID3D11Resource**>(&res));

	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, 0);
	V(SaveWICTextureToFileInMemory(deviceContext, res, ImageFormatMethods::ToGuid(format), hGlobal));
	res->Release();

	void* ptr = GlobalLock(hGlobal);
	uint size = GlobalSize(hGlobal);

	file << size;
	file.Write(ptr, size);

	GlobalUnlock(hGlobal);
	GlobalFree(hGlobal);
	return size;
}

//=================================================================================================
void Render::SaveScreenshot(cstring path, ImageFormat format)
{
	ID3D11Texture2D* backBuffer;
	V(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

	V(SaveWICTextureToFile(deviceContext, backBuffer, ImageFormatMethods::ToGuid(format), ToWString(path)));

	backBuffer->Release();
}

//=================================================================================================
bool Render::SetFeatureLevel(const string& level)
{
	if(initialized)
		return false;
	if(level == "10.0")
		forceFeatureLevel = D3D_FEATURE_LEVEL_10_0;
	else if(level == "10.1")
		forceFeatureLevel = D3D_FEATURE_LEVEL_10_1;
	else if(level == "11.0")
		forceFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	else
		return false;
	return true;
}
