#pragma once

//-----------------------------------------------------------------------------
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
struct Resolution
{
	Int2 size;
	uint hz;
};

//-----------------------------------------------------------------------------
struct CompileShaderParams
{
	cstring name;
	cstring cache_name;
	string* input;
	FileTime file_time;
	//D3DXMACRO* macros;
	//ID3DXEffectPool* pool;
	FIXME;
};

//-----------------------------------------------------------------------------
class Render
{
public:
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
	bool Reset(bool force);
	void WaitReset();
	void Clear(const Vec4& color);
	void Present();
	bool CheckDisplay(const Int2& size, int& hz); // dla zera zwraca najlepszy hz
	void RegisterShader(ShaderHandler* shader);
	void ReloadShaders();
	//ID3DXEffect* CompileShader(cstring name);
	//ID3DXEffect* CompileShader(CompileShaderParams& params);
	FIXME;
	TEX CreateTexture(const Int2& size, const Color* fill = nullptr);
	DynamicTexture* CreateDynamicTexture(const Int2& size);
	void CreateDynamicTexture(DynamicTexture* tex);
	RenderTarget* CreateRenderTarget(const Int2& size);
	void CreateRenderTargetTexture(RenderTarget* target);
	Texture* CopyToTexture(RenderTarget* target);
	TEX CopyToTextureRaw(RenderTarget* target, Int2 size = Int2::Zero);
	void AddManagedResource(ManagedResource* res) { managed_res.push_back(res); }
	bool IsLostDevice() const { return lost_device; }
	bool IsMultisamplingEnabled() const { return multisampling != 0; }
	bool IsVsyncEnabled() const { return vsync; }
	ID3D11Device* GetDevice() const { return device; }
	ID3D11DeviceContext* GetDeviceContext() const { return deviceContext; }
	void GetMultisampling(int& ms, int& msq) const { ms = multisampling; msq = multisampling_quality; }
	void GetResolutions(vector<Resolution>& v) const;
	void GetMultisamplingModes(vector<Int2>& v) const;
	int GetRefreshRate() const { return refresh_hz; }
	int GetShaderVersion() const { return shader_version; }
	int GetAdapter() const { return used_adapter; }
	const string& GetShadersDir() const { return shaders_dir; }
	//IDirect3DVertexDeclaration9* GetVertexDeclaration(VertexDeclarationId id) { return vertex_decl[id]; }
	FIXME;
	void SetAlphaBlend(bool use_alphablend);
	void SetAlphaTest(bool use_alphatest);
	void SetNoCulling(bool use_nocull);
	void SetNoZWrite(bool use_nozwrite);
	void SetVsync(bool vsync);
	int SetMultisampling(int type, int quality);
	void SetRefreshRateInternal(int refresh_hz) { this->refresh_hz = refresh_hz; }
	void SetShaderVersion(int shader_version) { this->shader_version = shader_version; }
	void SetTarget(RenderTarget* target);
	void SetTextureAddressMode(TextureAddressMode mode);
	void SetShadersDir(cstring dir) { shaders_dir = dir; }
	void SetAdapter(int adapter) { assert(!initialized); used_adapter = adapter; }
	//
	template<typename T> ID3D11Buffer* CreateConstantBuffer() { return CreateConstantBuffer(alignto(sizeof(T), 16)); }
	ID3D11Buffer* CreateConstantBuffer(uint size);
	ID3D11SamplerState* CreateSampler(TextureAddressMode mode = TEX_ADR_WRAP);
	void CreateShader(cstring filename, D3D11_INPUT_ELEMENT_DESC* input, uint inputCount, ID3D11VertexShader*& vertexShader, ID3D11PixelShader*& pixelShader,
		ID3D11InputLayout*& layout, D3D_SHADER_MACRO* macro = nullptr, cstring vsEntry = "VsMain", cstring psEntry = "PsMain");

private:
	void LogMultisampling();
	void LogAndSelectResolution();
	void SetDefaultRenderState();
	void CreateVertexDeclarations();
	void BeforeReset();
	void AfterReset();

	//-------------------
	void CreateAdapter();
	void CreateDeviceAndSwapChain();
	void CreateSizeDependentResources();
	void CreateRenderTarget();
	void CreateDepthStencilView();
	void SetViewport();
	ID3DBlob* CompileShader(cstring filename, cstring entry, bool isVertex, D3D_SHADER_MACRO* macro);
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGISwapChain* swap_chain;
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	ID3D11RenderTargetView* render_target;
	ID3D11DepthStencilView* depth_stencil_view;
	Int2 wnd_size;
	//-------------------
	vector<ShaderHandler*> shaders;
	vector<ManagedResource*> managed_res;
	//IDirect3DVertexDeclaration9* vertex_decl[VDI_MAX];
	RenderTarget* current_target;
	//SURFACE current_surf;
	FIXME;
	string shaders_dir;
	int used_adapter, shader_version, refresh_hz, multisampling, multisampling_quality;
	bool initialized, vsync, lost_device, res_freed, r_alphatest, r_nozwrite, r_nocull, r_alphablend;
};
