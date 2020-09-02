#pragma once

//-----------------------------------------------------------------------------
#include <QuadTree.h>
#include <Scene.h>

//-----------------------------------------------------------------------------
struct QuadtreeScene : public Scene
{
	struct Node : public QuadTree::Node
	{
		vector<SceneNode*> nodes;
	};

	QuadtreeScene(const Box2d& box, int splits);
	~QuadtreeScene();
	void Add(SceneNode* node) override;
	void Remove(SceneNode* node) override;
	void Clear() override;
	void ListNodes(SceneBatch& batch) override;

private:
	QuadTree quadtree;
	Node* root;
};
