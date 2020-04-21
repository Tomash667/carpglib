#include "Pch.h"
#include "SceneNode.h"

#include "Camera.h"
#include "ResourceManager.h"
#include "SceneManager.h"

//=================================================================================================
void SceneNode::SetMesh(Mesh* mesh, MeshInstance* mesh_inst)
{
	assert(mesh);
	this->mesh = mesh;
	this->mesh_inst = mesh_inst;
	flags = mesh_inst ? F_ANIMATED : 0;
	mesh->EnsureIsLoaded();
	radius = mesh->head.radius;
}

//=================================================================================================
void SceneNode::SetMesh(MeshInstance* mesh_inst)
{
	assert(mesh_inst);
	this->mesh = mesh_inst->mesh;
	this->mesh_inst = mesh_inst;
	flags = F_ANIMATED;
	mesh->EnsureIsLoaded();
	radius = mesh->head.radius;
}

//=================================================================================================
void SceneBatch::Clear()
{
	nodes.clear();
	alpha_nodes.clear();
	node_groups.clear();
}

//=================================================================================================
void SceneBatch::Add(SceneNode* node, int sub)
{
	assert(node && node->mesh && node->mesh->IsLoaded());

	const Mesh& mesh = *node->mesh;
	if(sub == -1)
	{
		assert(mesh.head.n_subs < 31);
		if(IsSet(mesh.head.flags, Mesh::F_ANIMATED))
			node->flags |= SceneNode::F_HAVE_WEIGHTS;
		if(IsSet(mesh.head.flags, Mesh::F_TANGENTS))
			node->flags |= SceneNode::F_HAVE_TANGENTS;
		if(app::scene_mgr->use_normalmap && IsSet(mesh.head.flags, Mesh::F_NORMAL_MAP))
			node->flags |= SceneNode::F_NORMAL_MAP;
		if(app::scene_mgr->use_specularmap && IsSet(mesh.head.flags, Mesh::F_SPECULAR_MAP))
			node->flags |= SceneNode::F_SPECULAR_MAP;
		node->subs = SceneNode::SPLIT_MASK;
	}
	else
	{
		if(app::scene_mgr->use_normalmap && mesh.subs[sub].tex_normal)
			node->flags |= SceneNode::F_NORMAL_MAP;
		if(app::scene_mgr->use_specularmap && mesh.subs[sub].tex_specular)
			node->flags |= SceneNode::F_SPECULAR_MAP;
		node->subs = SceneNode::SPLIT_INDEX | sub;
	}

	if(IsSet(node->flags, SceneNode::F_ALPHA_BLEND))
	{
		node->dist = Vec3::DistanceSquared(node->center, camera->from);
		alpha_nodes.push_back(node);
	}
	else
		nodes.push_back(node);
}

//=================================================================================================
void SceneBatch::Process()
{
	node_groups.clear();
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
		int prev_flags = -1, index = 0;
		for(SceneNode* node : nodes)
		{
			if(node->flags != prev_flags)
			{
				if(!node_groups.empty())
					node_groups.back().end = index - 1;
				node_groups.push_back({ node->flags, index, 0 });
				prev_flags = node->flags;
			}
			++index;
		}
		node_groups.back().end = index - 1;
	}

	// sort alpha nodes
	std::sort(alpha_nodes.begin(), alpha_nodes.end(), [](const SceneNode* node1, const SceneNode* node2)
	{
		return node1->dist > node2->dist;
	});
}
