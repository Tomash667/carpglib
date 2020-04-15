#pragma once

//-----------------------------------------------------------------------------
struct DebugNode : public ObjectPoolProxy<DebugNode>
{
	enum Mesh
	{
		None = -1,
		Box,
		Sphere,
		Capsule,
		Cylinder,
		TriMesh
	} mesh;
	Color color;
	Matrix mat;
	void* mesh_ptr;
};
