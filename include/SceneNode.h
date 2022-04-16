#pragma once

//-----------------------------------------------------------------------------
#include "MeshInstance.h"

//-----------------------------------------------------------------------------
struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	static constexpr int SPLIT_INDEX = 1 << 31;
	static constexpr int SPLIT_MASK = 0x7FFFFFFF;

	enum Flags
	{
		F_CUSTOM = 1 << 0,
		F_ANIMATED = 1 << 1,
		F_SPECULAR_MAP = 1 << 2,
		F_HAVE_TANGENTS = 1 << 3,
		F_NORMAL_MAP = 1 << 4,
		F_NO_ZWRITE = 1 << 5,
		F_NO_CULLING = 1 << 6,
		F_NO_LIGHTING = 1 << 7,
		F_ALPHA_BLEND = 1 << 8,
		F_HAVE_WEIGHTS = 1 << 9
	};

	Matrix mat;
	Mesh* mesh;
	MeshInstance* mesh_inst;
	int flags, subs;
	float radius, dist;
	const TexOverride* tex_override;
	Vec4 tint;
	Vec3 pos, rot, scale;
	array<Light*, 3> lights;
	bool visible;

	void OnGet();
	void OnFree();
	void SetMesh(Mesh* mesh, MeshInstance* mesh_inst = nullptr);
	void SetMesh(MeshInstance* mesh_inst);
	void UpdateMatrix();
};

//-----------------------------------------------------------------------------
struct SceneNodeGroup
{
	int flags, start, end;
};

//-----------------------------------------------------------------------------
struct SceneBatch
{
	vector<SceneNode*> nodes;
	vector<SceneNode*> alpha_nodes;
	vector<SceneNodeGroup> node_groups;
	Scene* scene;
	Camera* camera;
	bool gather_lights;

	void Clear();
	void Add(SceneNode* node, int sub = -1);
	void Process();
};

//-----------------------------------------------------------------------------
struct GlowNode
{
	SceneNode* node;
	Color color;
};

//-----------------------------------------------------------------------------
struct Decal
{
	Vec3 pos;
	Vec3 normal;
	float rot, scale;
	TEX tex;
	const std::array<Light*, 3>* lights;
};
