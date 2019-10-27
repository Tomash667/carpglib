#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
class BasicShader : public ShaderHandler
{
public:
	BasicShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	void Prepare(const CameraBase& camera);
	void BeginBatch();
	void AddQuad(const Vec3(&pts)[4], const Vec4& color);
	void EndBatch();

	ID3DXEffect* effect;
	D3DXHANDLE techSimple, techColor, techArea;
	D3DXHANDLE hMatCombined, hColor, hPlayerPos, hRange;

private:
	IDirect3DDevice9* device;
	VB vb;
	uint vb_size;
	vector<VColor> verts;
	Matrix mat_view_proj;
	bool batch;
};
