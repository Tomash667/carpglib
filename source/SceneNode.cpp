#include "Pch.h"
#include "SceneNode.h"

#include "Camera.h"
#include "ResourceManager.h"
#include "SceneManager.h"

//=================================================================================================
void SceneNode::OnGet()
{
	tex_override = nullptr;
	mesh_inst = nullptr;
	tint = Vec4::One;
	visible = true;
	dynamic = true;
	subs = SPLIT_MASK;
}

//=================================================================================================
void SceneNode::OnFree()
{
	SceneNode::Free(childs);
	if(mesh_inst && mesh_inst->mesh == mesh)
		delete mesh_inst;
}

//=================================================================================================
void SceneNode::Add(SceneNode* child, Mesh::Point* point)
{
	assert(child);
	if(point)
		assert(mesh->HavePoint(point));
	child->point = point;
	childs.push_back(child);
}

//=================================================================================================
SceneNode* SceneNode::GetChild(int id)
{
	for(SceneNode* child : childs)
	{
		if(child->id == id)
			return child;
	}
	return nullptr;
}

//=================================================================================================
void SceneNode::SetMesh(Mesh* mesh, MeshInstance* mesh_inst)
{
	assert(mesh);
	this->mesh = mesh;
	this->mesh_inst = mesh_inst;
	flags = mesh_inst ? F_ANIMATED : 0;
	mesh->EnsureIsLoaded();
	if(IsSet(mesh->head.flags, Mesh::F_ANIMATED))
		flags |= F_HAVE_WEIGHTS;
	if(IsSet(mesh->head.flags, Mesh::F_TANGENTS))
		flags |= F_HAVE_TANGENTS;
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
	if(IsSet(mesh->head.flags, Mesh::F_ANIMATED))
		flags |= F_HAVE_WEIGHTS;
	if(IsSet(mesh->head.flags, Mesh::F_TANGENTS))
		flags |= F_HAVE_TANGENTS;
	radius = mesh->head.radius;
}

//=================================================================================================
// Reuse existing mesh instance if possible
void SceneNode::ReplaceMesh(Mesh* mesh)
{
	assert(mesh);
	mesh->EnsureIsLoaded();
	if(mesh_inst)
	{
		assert(this->mesh->head.n_groups == mesh->head.n_groups && this->mesh->head.n_bones == mesh->head.n_bones);
		mesh_inst->mesh = mesh;
	}
	else
		mesh_inst = new MeshInstance(mesh);
	this->mesh = mesh;
	flags = F_ANIMATED;
	if(IsSet(mesh->head.flags, Mesh::F_ANIMATED))
		flags |= F_HAVE_WEIGHTS;
	if(IsSet(mesh->head.flags, Mesh::F_TANGENTS))
		flags |= F_HAVE_TANGENTS;
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
void SceneBatch::Add(SceneNode* node)
{
	assert(node && node->mesh && node->mesh->IsLoaded());

	const Mesh& mesh = *node->mesh;
	if(!IsSet(node->subs, SceneNode::SPLIT_INDEX))
	{
		assert(mesh.head.n_subs < 31);
		if(app::scene_mgr->use_normalmap && IsSet(mesh.head.flags, Mesh::F_NORMAL_MAP))
			node->flags |= SceneNode::F_NORMAL_MAP;
		if(app::scene_mgr->use_specularmap && IsSet(mesh.head.flags, Mesh::F_SPECULAR_MAP))
			node->flags |= SceneNode::F_SPECULAR_MAP;
	}
	else
	{
		int index = (node->subs & ~SceneNode::SPLIT_INDEX);
		if(app::scene_mgr->use_normalmap && mesh.subs[index].tex_normal)
			node->flags |= SceneNode::F_NORMAL_MAP;
		if(app::scene_mgr->use_specularmap && mesh.subs[index].tex_specular)
			node->flags |= SceneNode::F_SPECULAR_MAP;
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
