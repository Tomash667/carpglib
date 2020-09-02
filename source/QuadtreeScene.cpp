#include "Pch.h"
#include "QuadtreeScene.h"

#include "SceneNode.h"

static ObjectPool<QuadtreeScene::Node> nodePool;

QuadtreeScene::QuadtreeScene(const Box2d& box, int splits)
{
	assert(splits >= 1);
	quadtree.get = [] { return nodePool.Get(); };
	quadtree.Init(nullptr, box, splits);
}

QuadtreeScene::~QuadtreeScene()
{
	Clear();
}

void QuadtreeScene::Add(SceneNode* node)
{
	assert(node);
	if(node->dynamic)
		nodes.push_back(node);
	else
	{

	}
}

void QuadtreeScene::Remove(SceneNode* node)
{

}

void QuadtreeScene::Clear()
{

}

void QuadtreeScene::ListNodes(SceneBatch& batch)
{

}
