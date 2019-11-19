#pragma once

//-----------------------------------------------------------------------------
#include "MeshInstance.h"

//-----------------------------------------------------------------------------
struct SceneNode : public ObjectPoolProxy<SceneNode>
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

	Scene* scene;
	Matrix mat;
	Mesh* mesh;
	MeshInstance* mesh_inst;
	float dist;
	int flags, lights, subs;
	const TexOverride* tex_override;
	Vec4 tint;
	Vec3 pos;
	bool billboard, tmp;

	void OnGet()
	{
		tex_override = nullptr;
		tint = Vec4::One;
		billboard = false;
		tmp = true;
	}
	void Remove();
};

//-----------------------------------------------------------------------------
struct SceneNodeGroup
{
	int flags, start, end;
};

//-----------------------------------------------------------------------------
struct SceneNodeBatch
{
	CameraBase* camera;
	vector<SceneNode*> nodes;
	vector<SceneNodeGroup> groups;
	vector<SceneNode*> alpha_nodes;

	void Clear();
	void Add(SceneNode* node);
	void Process();
};
