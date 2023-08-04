#include "Pch.h"
#include "SimpleMesh.h"

#include "VertexData.h"
#include "DirectX.h"

//=================================================================================================
SimpleMesh::~SimpleMesh()
{
	SafeRelease(vb);
	SafeRelease(ib);
}

//=================================================================================================
void SimpleMesh::Build()
{
	if(vb)
		return;

	// use VertexData or members?
	Vec3* verticesData;
	word* indicesData;
	uint verticesCount, indicesCount;
	if(vd)
	{
		verticesData = vd->verts.data();
		indicesData = reinterpret_cast<word*>(vd->faces.data());
		verticesCount = vd->verts.size();
		indicesCount = vd->faces.size() * 3;
	}
	else
	{
		verticesData = vertices.data();
		indicesData = indices.data();
		verticesCount = vertices.size();
		indicesCount = indices.size();
	}

	// create vertex buffer
	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = sizeof(Vec3) * verticesCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = verticesData;

	V(app::render->GetDevice()->CreateBuffer(&desc, &data, &vb));
	SetDebugName(vb, "SimpleMeshVb");

	// create index buffer
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = sizeof(word) * indicesCount;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	data.pSysMem = indicesData;

	V(app::render->GetDevice()->CreateBuffer(&desc, &data, &ib));
	SetDebugName(ib, "SimpleMeshIb");

	indexCount = indicesCount;
}

//=================================================================================================
void SimpleMesh::Clear()
{
	SafeRelease(vb);
	SafeRelease(ib);
	vertices.clear();
	indices.clear();
}
