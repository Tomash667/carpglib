#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"
#include "Mesh.h"

//-----------------------------------------------------------------------------
class SuperShader : public ShaderHandler
{
	enum Switches
	{
		HAVE_WEIGHT = 1 << 0,
		HAVE_TANGENTS = 1 << 1,
		ANIMATED = 1 << 2,
		FOG = 1 << 3,
		SPECULAR_MAP = 1 << 4,
		NORMAL_MAP = 1 << 5,
		POINT_LIGHT = 1 << 6,
		DIR_LIGHT = 1 << 7
	};

	struct Shader
	{
		uint id;
		ID3D11VertexShader* vertexShader;
		ID3D11PixelShader* pixelShader;
		ID3D11InputLayout* layout;
	};

public:
	SuperShader();
	cstring GetName() const override { return "super"; }
	void OnInit() override;
	void OnRelease() override;
	uint GetShaderId(bool have_weights, bool have_tangents, bool animated, bool fog, bool specular_map, bool normal_map, bool point_light, bool dir_light) const;
	void SetScene(Scene* scene, Camera* camera);
	void Prepare();
	void PrepareDecals();
	void SetShader(uint id);
	void SetTexture(const TexOverride* texOverride, Mesh* mesh, uint index);
	void SetCustomMesh(ID3D11Buffer* vb, ID3D11Buffer* ib, uint vertexSize);
	void Draw(SceneNode* node);
	void DrawCustom(const Matrix& matWorld, const Matrix& matCombined, const std::array<Light*, 3>& lights, uint startIndex, uint indexCount);
	void DrawDecal(const Decal& decal);

private:
	Shader& GetShader(uint id);
	Shader& CompileShader(uint id);
	void DrawSubmesh(SceneNode* node, uint index);

	ID3D11DeviceContext* deviceContext;
	vector<Shader> shaders;
	ID3D11SamplerState* samplerDiffuse;
	ID3D11SamplerState* samplerNormal;
	ID3D11SamplerState* samplerSpecular;
	ID3D11Buffer* vsGlobals;
	ID3D11Buffer* vsLocals;
	ID3D11Buffer* psGlobals;
	ID3D11Buffer* psLocals;
	ID3D11Buffer* psMaterial;
	TEX texEmptyNormalMap, texEmptySpecularMap;
	ID3D11Buffer* vbDecal;
	ID3D11Buffer* ibDecal;

	Camera* camera;
	bool applyBones, applyLights, applyNormalMap, applySpecularMap;
	Mesh* prevMesh;
};
