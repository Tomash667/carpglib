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
	/*ID3DXEffect* GetShader(uint id);
	ID3DXEffect* CompileShader(uint id);
	ID3DXEffect* GetEffect() const { return shaders.front().e; }*/
	void ApplyLights(const array<Light*, 3>& lights);

	void Prepare(Scene* scene, Camera* camera);
	void SetShader(uint id);
	void Draw(SceneNode* node);

private:
	Shader& GetShader(uint id);
	Shader& CompileShader(uint id);
	void DrawSubmesh(Mesh::Submesh& sub);

	/*string code;
	FileTime edit_time;*/
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

	Camera* camera;
	bool applyBones, applyLights, applyNormalMap, applySpecularMap;
	Mesh* prevMesh;
};
