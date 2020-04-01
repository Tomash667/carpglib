#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class SuperShader : public ShaderHandler
{
	enum Switches
	{
		ANIMATED,
		HAVE_TANGENTS,
		FOG,
		SPECULAR,
		NORMAL,
		POINT_LIGHT,
		DIR_LIGHT
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
	~SuperShader();
	cstring GetName() const override { return "super"; }
	void OnInit() override;
	void OnRelease() override;
	uint GetShaderId(bool animated, bool have_tangents, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const;
	/*ID3DXEffect* GetShader(uint id);
	ID3DXEffect* CompileShader(uint id);
	ID3DXEffect* GetEffect() const { return shaders.front().e; }*/
	void ApplyLights(const array<Light*, 3>& lights);

	TEX tex_empty_normal_map, tex_empty_specular_map;

private:
	string code;
	FileTime edit_time;
	vector<Shader> shaders;
};
