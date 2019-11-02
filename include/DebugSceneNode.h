#pragma once

//-----------------------------------------------------------------------------
struct DebugSceneNode
{
	enum Type
	{
		Box,
		Cylinder,
		Sphere,
		Capsule,
		TriMesh,
		MaxType
	} type;
	Matrix mat;
	Color color;
	void* mesh_ptr;
};
