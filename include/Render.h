#pragma once

//-----------------------------------------------------------------------------
#include "ImageFormat.h"

//-----------------------------------------------------------------------------
struct Resolution
{
	Int2 size;

	bool operator < (const Resolution& r) const
	{
		if(size.x > r.size.x)
			return false;
		else if(size.x < r.size.x)
			return true;
		else if(size.y > r.size.y)
			return false;
		else if(size.y < r.size.y)
			return true;
		else
			return false;
	}
};

//-----------------------------------------------------------------------------
class Render
{
public:
	enum BlendState
	{
		BLEND_NO,
		BLEND_ADD,
		BLEND_ADD_ONE,
		BLEND_ADD_ONE2,
		BLEND_REV_SUBTRACT,
		BLEND_MAX
	};

	enum DepthState
	{
		DEPTH_NO,
		DEPTH_READ,
		DEPTH_YES,
		DEPTH_MAX
	};

	enum RasterState
	{
		RASTER_NORMAL,
		RASTER_NO_CULLING,
		RASTER_WIREFRAME,
		RASTER_MAX
	};

	enum TextureAddressMode
	{
		TEX_ADR_WRAP = 1,
		TEX_ADR_MIRROR = 2,
		TEX_ADR_CLAMP = 3,
		TEX_ADR_BORDER = 4,
		TEX_ADR_MIRRORONCE = 5
	};

	struct ShaderParams
	{
		cstring name;
		cstring cacheName;
		cstring vsEntry;
		cstring psEntry;
		const string* code;
		VertexDeclarationId decl;
		ID3D11VertexShader** vertexShader;
		ID3D11PixelShader** pixelShader;
		ID3D11InputLayout** layout;
		D3D_SHADER_MACRO* macro;
		ID3DBlob** vsBlob;

		ShaderParams() : name(nullptr), cacheName(nullptr), vsEntry("VsMain"), psEntry("PsMain"), code(nullptr), vertexShader(nullptr),
			pixelShader(nullptr), layout(nullptr), macro(nullptr), vsBlob(nullptr)
		{
		}
	};

	Render();
	~Render();
	void Init();
	void OnChangeResolution();

	bool CheckDisplay(const Int2& size) const;
	void Clear(const Vec4& color);
	Texture* CopyToTexture(RenderTarget* target);
	TEX CopyToTextureRaw(RenderTarget* target);
	ID3D11Buffer* CreateConstantBuffer(uint size, cstring name = nullptr);
	DynamicTexture* CreateDynamicTexture(const Int2& size);
	TEX CreateImmutableTexture(const Int2& size, const Color* fill);
	ID3D11InputLayout* CreateInputLayout(VertexDeclarationId decl, ID3DBlob* vsBlob, cstring name);
	RenderTarget* CreateRenderTarget(const Int2& size, int flags = 0);
	ID3D11SamplerState* CreateSampler(TextureAddressMode mode = TEX_ADR_WRAP, bool disableMipmap = false);
	void CreateShader(ShaderParams& params);
	void Present();
	void RegisterShader(ShaderHandler* shader);
	void ReloadShaders();
	void SaveToFile(TEX tex, cstring path, ImageFormat format = ImageFormat::JPG);
	uint SaveToFile(TEX tex, FileWriter& file, ImageFormat format = ImageFormat::JPG);
	void SaveScreenshot(cstring path, ImageFormat format = ImageFormat::JPG);

	bool IsMultisamplingEnabled() const { return multisampling != 0; }
	bool IsVsyncEnabled() const { return vsync; }

	int GetAdapter() const { return usedAdapter; }
	ID3D11DepthStencilView* GetDepthStencilView() const { return depthStencilView; }
	ID3D11Device* GetDevice() const { return device; }
	ID3D11DeviceContext* GetDeviceContext() const { return deviceContext; }
	void GetMultisampling(int& ms, int& msq) const { ms = multisampling; msq = multisamplingQuality; }
	const vector<Int2>& GetMultisamplingModes() const { return multisampleLevels; }
	RenderTarget* GetRenderTarget() const { return currentTarget; }
	ID3D11RenderTargetView* GetRenderTargetView() const { return renderTargetView; }
	const vector<Resolution>& GetResolutions() const { return resolutions; }
	ID3D11SamplerState* GetSampler() const { return defaultSampler; }
	const string& GetShadersDir() const { return shaders_dir; }

	void SetAdapter(int adapter) { assert(!initialized); usedAdapter = adapter; }
	void SetBlendState(BlendState blendState);
	void SetDepthState(DepthState depthState);
	bool SetFeatureLevel(const string& level);
	void SetRasterState(RasterState rasterState);
	int SetMultisampling(int type, int quality);
	void SetShadersDir(cstring dir) { shaders_dir = dir; }
	void SetRenderTarget(RenderTarget* target);
	void SetViewport(const Int2& size);
	void SetVsync(bool vsync) { this->vsync = vsync; }

private:
	void CreateAdapter();
	void CreateDevice();
	void CreateSwapChain();
	void CreateSizeDependentResources();
	void CreateRenderTargetView();
	ID3D11DepthStencilView* CreateDepthStencilView(const Int2& size, bool useMs = true);
	void CreateBlendStates();
	void CreateDepthStates();
	void CreateRasterStates();
	void LogAndSelectResolution();
	void LogAndSelectMultisampling();
	ID3DBlob* CompileShader(ShaderParams& params, bool isVertex);
	void RecreateRenderTarget(RenderTarget* target);
	void CreateRenderTargetInternal(RenderTarget* target);

	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGISwapChain* swapChain;
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	ID3D11RenderTargetView* renderTargetView;
	ID3D11DepthStencilView* depthStencilView;
	ID3D11BlendState* blendStates[BLEND_MAX];
	ID3D11DepthStencilState* depthStates[DEPTH_MAX];
	ID3D11RasterizerState* rasterStates[RASTER_MAX];
	ID3D11SamplerState* defaultSampler;
	Int2 wndSize;
	vector<ShaderHandler*> shaders;
	vector<RenderTarget*> renderTargets;
	vector<Resolution> resolutions;
	vector<Int2> multisampleLevels;
	RenderTarget* currentTarget;
	string shaders_dir;
	int usedAdapter, multisampling, multisamplingQuality, forceFeatureLevel;
	BlendState blendState;
	DepthState depthState;
	RasterState rasterState;
	bool initialized, vsync, useV4Shaders;
};
