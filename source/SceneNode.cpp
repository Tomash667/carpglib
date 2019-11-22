#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneNode.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "CameraBase.h"
#include "Scene.h"

//=================================================================================================
void SceneNode::Remove()
{
	assert(scene);
	scene->Remove(this);
}

//=================================================================================================
void SceneNode::SetMesh(Mesh* mesh)
{
	assert(mesh);
	if(mesh->state != ResourceState::Loaded)
		app::res_mgr->LoadInstant(const_cast<Mesh*>(mesh));
	this->mesh = mesh;
	ApplyFlags();
}

//=================================================================================================
void SceneNode::SetMeshInstance(Mesh* mesh)
{
	assert(mesh);
	if(mesh->state != ResourceState::Loaded)
		app::res_mgr->LoadInstant(const_cast<Mesh*>(mesh));
	this->mesh = mesh;
	this->mesh_inst = new MeshInstance(mesh);
	ApplyFlags();
}

//=================================================================================================
void SceneNode::SetMeshInstance(MeshInstance* mesh_inst)
{
	assert(mesh_inst && mesh_inst->mesh && mesh_inst->mesh->IsLoaded());
	this->mesh = mesh_inst->mesh;
	this->mesh_inst = mesh_inst;
	ApplyFlags();
}

//=================================================================================================
void SceneNode::SetParentMeshInstance(MeshInstance* mesh_inst)
{
	assert(mesh_inst);
	this->mesh_inst = mesh_inst;
	flags |= F_ANIMATED;
}

//=================================================================================================
void SceneNode::ApplyFlags()
{
	flags = 0;
	if(mesh_inst)
		flags |= SceneNode::F_ANIMATED;
	if(IsSet(mesh->head.flags, Mesh::F_TANGENTS))
		flags |= SceneNode::F_TANGENTS;
}

//=================================================================================================
void SceneNodeBatch::Clear()
{
	LoopAndRemove(nodes, [](SceneNode* node) { return node->type != SceneNode::TEMPORARY; });
	LoopAndRemove(alpha_nodes, [](SceneNode* node) { return node->type != SceneNode::TEMPORARY; });
	SceneNode::Free(nodes);
	SceneNode::Free(alpha_nodes);
	groups.clear();
}

//=================================================================================================
void SceneNodeBatch::Add(SceneNode* node)
{
	assert(node && node->mesh && node->mesh->head.n_subs < 31);

	const Mesh& mesh = *node->mesh;
	if(app::scene_mgr->use_normalmap && IsSet(mesh.head.flags, Mesh::F_NORMAL_MAP))
		node->flags |= SceneNode::F_NORMAL_MAP;
	if(app::scene_mgr->use_specularmap && IsSet(mesh.head.flags, Mesh::F_SPECULAR_MAP))
		node->flags |= SceneNode::F_SPECULAR_MAP;
	node->subs = 0x7FFFFFFF;

	if(node->type == SceneNode::BILLBOARD)
		node->mat = Matrix::CreateLookAt(node->pos, camera->from);

	if(IsSet(node->flags, SceneNode::F_ALPHA_BLEND))
	{
		node->dist = Vec3::DistanceSquared(node->pos, camera->from);
		alpha_nodes.push_back(node);
	}
	else
		nodes.push_back(node);
}

//=================================================================================================
void SceneNodeBatch::Process()
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
	groups.clear();
	int prev_flags = -1, index = 0;
	for(SceneNode* node : nodes)
	{
		if(node->flags != prev_flags)
		{
			if(!groups.empty())
				groups.back().end = index - 1;
			groups.push_back({ node->flags, index, 0 });
			prev_flags = node->flags;
		}
		++index;
	}
	groups.back().end = index - 1;

	// sort alpha nodes
	std::sort(alpha_nodes.begin(), alpha_nodes.end(), [](const SceneNode* node1, const SceneNode* node2)
		{
			return node1->dist > node2->dist;
		});
}
