#include "Pch.h"
#include "SceneNode.h"

#include "Camera.h"
#include "ResourceManager.h"
#include "SceneManager.h"

//=================================================================================================
void SceneNode::OnGet()
{
	pos = Vec3::Zero;
	rot = Vec3::Zero;
	scale = Vec3::One;
	tint = Vec4::One;
	meshInst = nullptr;
	texOverride = nullptr;
	subs = SPLIT_MASK;
	visible = true;
}

//=================================================================================================
void SceneNode::OnFree()
{
	if(meshInstOwner)
		delete meshInst;
}

//=================================================================================================
void SceneNode::SetMesh(Mesh* mesh, MeshInstance* meshInst)
{
	assert(mesh);
	this->mesh = mesh;
	this->meshInst = meshInst;
	meshInstOwner = false;
	UpdateFlags();
}

//=================================================================================================
void SceneNode::SetMesh(MeshInstance* meshInst)
{
	assert(meshInst);
	this->mesh = meshInst->GetMesh();
	this->meshInst = meshInst;
	meshInstOwner = true;
	UpdateFlags();
}

//=================================================================================================
void SceneNode::UpdateFlags()
{
	mesh->EnsureIsLoaded();
	radius = mesh->head.radius;
	flags = meshInst ? F_ANIMATED : 0;
	if(IsSet(mesh->head.flags, Mesh::F_ANIMATED))
		flags |= SceneNode::F_HAVE_WEIGHTS;
	if(IsSet(mesh->head.flags, Mesh::F_TANGENTS))
		flags |= SceneNode::F_HAVE_TANGENTS;
}

//=================================================================================================
void SceneNode::UpdateMatrix()
{
	mat = Matrix::Transform(pos, rot, scale);
}

//=================================================================================================
void SceneBatch::Clear()
{
	nodes.clear();
	alphaNodes.clear();
	nodeGroups.clear();
	particleEmitters.clear();
}

//=================================================================================================
void SceneBatch::Add(SceneNode* node)
{
	assert(node && node->mesh && node->mesh->IsLoaded());

	const Mesh& mesh = *node->mesh;
	if(!IsSet(node->subs, SceneNode::SPLIT_INDEX))
	{
		assert(mesh.head.nSubs < 31);
		if(app::sceneMgr->useNormalmap && IsSet(mesh.head.flags, Mesh::F_NORMAL_MAP))
			node->flags |= SceneNode::F_NORMAL_MAP;
		if(app::sceneMgr->useSpecularmap && IsSet(mesh.head.flags, Mesh::F_SPECULAR_MAP))
			node->flags |= SceneNode::F_SPECULAR_MAP;
	}
	else
	{
		int sub = node->subs & SceneNode::SPLIT_MASK;
		if(app::sceneMgr->useNormalmap && mesh.subs[sub].texNormal)
			node->flags |= SceneNode::F_NORMAL_MAP;
		if(app::sceneMgr->useSpecularmap && mesh.subs[sub].texSpecular)
			node->flags |= SceneNode::F_SPECULAR_MAP;
	}

	if(node->meshInst)
		node->meshInst->SetupBones();

	if(IsSet(node->flags, SceneNode::F_ALPHA_BLEND))
	{
		node->dist = Vec3::DistanceSquared(node->pos, camera->from);
		alphaNodes.push_back(node);
	}
	else
		nodes.push_back(node);
}

//=================================================================================================
void SceneBatch::Process()
{
	nodeGroups.clear();
	if(!nodes.empty())
	{
		// sort nodes
		std::sort(nodes.begin(), nodes.end(), [](const SceneNode* node1, const SceneNode* node2)
		{
			if(node1->flags == node2->flags)
				return node1->mesh > node2->mesh;
			else
				return node1->flags < node2->flags;
		});

		// group nodes
		int prevFlags = -1, index = 0;
		for(SceneNode* node : nodes)
		{
			if(node->flags != prevFlags)
			{
				if(!nodeGroups.empty())
					nodeGroups.back().end = index - 1;
				nodeGroups.push_back({ node->flags, index, 0 });
				prevFlags = node->flags;
			}
			++index;
		}
		nodeGroups.back().end = index - 1;
	}

	// sort alpha nodes
	std::sort(alphaNodes.begin(), alphaNodes.end(), [](const SceneNode* node1, const SceneNode* node2)
	{
		return node1->dist > node2->dist;
	});
}
