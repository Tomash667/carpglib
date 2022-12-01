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
	MeshInstance* meshInst;
	int flags, subs;
	float radius, dist;
	const TexOverride* texOverride;
	Vec4 tint;
	Vec3 center;
	array<Light*, 3> lights;
	bool persistent;

	void OnGet();
	void SetMesh(Mesh* mesh, MeshInstance* meshInst = nullptr);
	void SetMesh(MeshInstance* meshInst);
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
	vector<SceneNode*> alphaNodes;
	vector<SceneNodeGroup> nodeGroups;
	Scene* scene;
	Camera* camera;
	bool gatherLights;

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
