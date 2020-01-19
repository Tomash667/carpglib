#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class SkyboxShader : public ShaderHandler
{
public:
	SkyboxShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	void Draw(Mesh& mesh, Camera& camera);

private:
	ID3DXEffect* effect;
	D3DXHANDLE tech;
	D3DXHANDLE hMatCombined, hTex;
};
