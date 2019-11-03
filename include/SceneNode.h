#pragma once

//-----------------------------------------------------------------------------
#include "MeshInstance.h"

//-----------------------------------------------------------------------------
struct SceneNode
{
	static constexpr int SPLIT_INDEX = 1 << 31;

	enum Flags
	{
		F_ANIMATED = 1 << 0,
		F_SPECULAR_MAP = 1 << 1,
		F_BINORMALS = 1 << 2,
		F_NORMAL_MAP = 1 << 3,
		F_NO_ZWRITE = 1 << 4,
		F_NO_CULLING = 1 << 5,
		F_ALPHA_TEST = 1 << 6,
		F_ALPHA_BLEND = 1 << 7,
		F_NO_LIGHTING = 1 << 8
	};

	Matrix mat;
	union
	{
		Mesh* mesh;
		MeshInstance* mesh_inst;
	};
	MeshInstance* parent_mesh_inst;
	float dist;
	int flags, lights, subs;
	const TexOverride* tex_override;
	Vec4 tint;
	Vec3 pos;
	bool billboard;

	const Mesh& GetMesh() const
	{
		if(!IsSet(flags, F_ANIMATED) || parent_mesh_inst)
			return *mesh;
		else
			return *mesh_inst->mesh;
	}

	const MeshInstance& GetMeshInstance() const
	{
		assert(IsSet(flags, F_ANIMATED));
		if(!parent_mesh_inst)
			return *mesh_inst;
		else
			return *parent_mesh_inst;
	}
};

//-----------------------------------------------------------------------------
struct SceneNodeGroup
{
	int flags, start, end;
};
