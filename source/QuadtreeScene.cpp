#include "Pch.h"
#include "QuadtreeScene.h"

#include "SceneNode.h"

static ObjectPool<QuadtreeScene::Node> nodePool;

QuadtreeScene::QuadtreeScene(const Box2d& box, int splits)
{
	assert(splits >= 1);
	quadtree.Init([]() -> QuadTree::Node* { return nodePool.Get(); }, box, splits);
	root = static_cast<Node*>(quadtree.GetRoot());
}

QuadtreeScene::~QuadtreeScene()
{
	quadtree.Clear([](QuadTree::Node* quadNode)
	{
		Node* node = static_cast<Node*>(quadNode);
		SceneNode::Free(node->nodes);
		nodePool.Free(node);
	});
}

void QuadtreeScene::Add(SceneNode* node)
{
	assert(node);
	if(node->dynamic)
		root->nodes.push_back(node);
	else
	{
		QuadTree::Node* quadNode = quadtree.GetNode(node->center.XZ(), node->radius);
		static_cast<Node*>(quadNode)->nodes.push_back(node);
	}
}

void QuadtreeScene::Remove(SceneNode* node)
{
	assert(node);
	if(node->dynamic)
		RemoveElement(root->nodes, node);
	else
	{
		QuadTree::Node* quadNode = quadtree.GetNode(node->center.XZ(), node->radius);
		RemoveElement(static_cast<Node*>(quadNode)->nodes, node);
	}
}

void QuadtreeScene::Clear()
{
	quadtree.ForEach([](QuadTree::Node* quadNode)
	{
		Node* node = static_cast<Node*>(quadNode);
		SceneNode::Free(node->nodes);
	});
}

void QuadtreeScene::ListNodes(SceneBatch& batch)
{
	bool any = quadtree.List(batch.frustum, [&](QuadTree::Node* node)
	{
		Scene::ListNodes(batch, static_cast<Node*>(node)->nodes);
	});

	if(!any)
		Scene::ListNodes(batch, root->nodes);
}
