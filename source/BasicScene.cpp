#include "Pch.h"
#include "BasicScene.h"

#include <Camera.h>
#include <SceneNode.h>

//=================================================================================================
BasicScene::~BasicScene()
{
	Clear();
}

//=================================================================================================
void BasicScene::Add(SceneNode* node)
{
	assert(node);
	nodes.push_back(node);
}

//=================================================================================================
void BasicScene::Remove(SceneNode* node)
{
	assert(node);
	RemoveElement(nodes, node);
}

//=================================================================================================
void BasicScene::Clear()
{
	SceneNode::Free(nodes);
	lights.clear();
}

//=================================================================================================
void BasicScene::ListNodes(SceneBatch& batch)
{
	FrustumPlanes frustum(batch.camera->mat_view_proj);
	for(SceneNode* node : nodes)
	{
		if(!node->visible)
			continue;
		if(node->mesh && frustum.SphereToFrustum(node->center, node->radius))
		{
			if(node->mesh_inst)
				node->mesh_inst->SetupBones();
			if(batch.gather_lights && !IsSet(node->flags, SceneNode::F_NO_LIGHTING))
				GatherLights(batch, node);
			batch.Add(node);
		}
		for(SceneNode* child : node->childs)
		{
			if(child->mesh && frustum.SphereToFrustum(child->center, child->radius))
			{
				if(child->mesh_inst)
					child->mesh_inst->SetupBones();
				if(batch.gather_lights && !IsSet(child->flags, SceneNode::F_NO_LIGHTING))
					GatherLights(batch, child);
				batch.Add(child);
			}
		}
	}
}