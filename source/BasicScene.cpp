#include "Pch.h"
#include "BasicScene.h"

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
	Scene::ListNodes(batch, nodes);
}
