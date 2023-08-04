#pragma once

//-----------------------------------------------------------------------------
struct SimpleMesh
{
	SimpleMesh() : vd(nullptr), vb(nullptr), ib(nullptr) {}
	~SimpleMesh();
	void Build();
	void Clear();

	VertexData* vd;
	ID3D11Buffer* vb;
	ID3D11Buffer* ib;
	vector<Vec3> vertices;
	vector<word> indices;
	uint indexCount;
};
