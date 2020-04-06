#pragma once

//-----------------------------------------------------------------------------
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
struct Resolution
{
	Int2 size;
	uint hz;

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
		else if(hz > r.hz)
			return false;
		else if(hz < r.hz)
			return true;
		else
			return false;
	}
};

//-----------------------------------------------------------------------------
class Render
{
public:
	enum DepthState
	{
		DEPTH_NO,
		DEPTH_READ,
		DEPTH_YES,
		DEPTH_MAX
	};

	enum TextureAddressMode
	{
		TEX_ADR_WRAP = 1,
		TEX_ADR_MIRROR = 2,
		TEX_ADR_CLAMP = 3,
		TEX_ADR_BORDER = 4,
		TEX_ADR_MIRRORONCE = 5
	};

	Render();
	~Render();
	void Init();

	bool CheckDisplay(const Int2& size, uint& hz); // for hz=0 return best hz available
	void Clear(const Vec4& color);
	Texture* CopyToTexture(RenderTarget* target);
	TEX CopyToTextureRaw(RenderTarget* target);
	void CreateShader(cstring filename, D3D11_INPUT_ELEMENT_DESC* input, uint inputCount, ID3D11VertexShader*& vertexShader, ID3D11PixelShader*& pixelShader,
		ID3D11InputLayout*& layout, D3D_SHADER_MACRO* macro = nullptr, cstring vsEntry = "VsMain", cstring psEntry = "PsMain");
	ID3D11Buffer* CreateConstantBuffer(uint size, cstring name = nullptr);
	TEX CreateRawTexture(const Int2& size, const Color* fill = nullptr);
	RenderTarget* CreateRenderTarget(const Int2& size);
	ID3D11SamplerState* CreateSampler(TextureAddressMode mode = TEX_ADR_WRAP, bool disableMipmap = false);
	Texture* CreateTexture(const Int2& size);
	void Present();
	void RegisterShader(ShaderHandler* shader);
	void ReloadShaders();

	bool IsMultisamplingEnabled() const { return multisampling != 0; }
	bool IsVsyncEnabled() const { return vsync; }

	int GetAdapter() const { return usedAdapter; }
	ID3D11Device* GetDevice() const { return device; }
	ID3D11DeviceContext* GetDeviceContext() const { return deviceContext; }
	ID3D11InputLayout* GetInputLayout(VertexDeclarationId decl);
	void GetMultisampling(int& ms, int& msq) const { ms = multisampling; msq = multisamplingQuality; }
	void GetMultisamplingModes(vector<Int2>& v) const;
	uint GetRefreshRate() const { return refreshHz; }
	const vector<Resolution>& GetResolutions() const { return resolutions; }
	const string& GetShadersDir() const { return shaders_dir; }

	void SetAdapter(int adapter) { assert(!initialized); usedAdapter = adapter; }
	void SetAlphaBlend(bool useAlphaBlend);
	void SetAlphaTest(bool use_alphatest);
	void SetDepthState(DepthState depthState);
	void SetRefreshRateInternal(uint refreshHz) { this->refreshHz = refreshHz; }
	int SetMultisampling(int type, int quality);
	void SetNoCulling(bool use_nocull);
	void SetShadersDir(cstring dir) { shaders_dir = dir; }
	void SetTarget(RenderTarget* target);
	void SetTextureAddressMode(TextureAddressMode mode);
	void SetVsync(bool vsync) { this->vsync = vsync; }

private:
	void CreateAdapter();
	void CreateDeviceAndSwapChain();
	void CreateSizeDependentResources();
	void CreateRenderTarget();
	void CreateDepthStencilView();
	void SetViewport();
	void CreateBlendStates();
	void CreateDepthStates();
	void CreateRasterStates();
	void LogAndSelectResolution();
	void LogMultisampling();
	ID3DBlob* CompileShader(cstring filename, cstring entry, bool isVertex, D3D_SHADER_MACRO* macro);
	ID3D11InputLayout* CreateInputLayout(VertexDeclarationId decl);

	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGISwapChain* swapChain;
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	ID3D11RenderTargetView* renderTarget;
	ID3D11DepthStencilView* depthStencilView;
	ID3D11BlendState* blendStates[2];
	ID3D11DepthStencilState* depthStates[DEPTH_MAX];
	ID3D11RasterizerState* rasterStates[2];
	ID3D11InputLayout* inputLayouts[VDI_MAX];
	Int2 wndSize;
	vector<ShaderHandler*> shaders;
	vector<RenderTarget*> renderTargets;
	vector<Resolution> resolutions;
	string shaders_dir;
	uint refreshHz;
	int usedAdapter, multisampling, multisamplingQuality;
	DepthState depthState;
	bool useAlphaBlend, useNoCull;
	bool initialized, vsync, r_alphatest;
};
