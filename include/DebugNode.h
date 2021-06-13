#pragma once

//-----------------------------------------------------------------------------
#include "MeshShape.h"

//-----------------------------------------------------------------------------
struct DebugNode : public ObjectPoolProxy<DebugNode>
{
	MeshShape shape;
	Color color;
	Matrix mat;
	SimpleMesh* trimesh;
};
