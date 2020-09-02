#pragma once

//-----------------------------------------------------------------------------
#include <QuadTree.h>
#include <Scene.h>

//-----------------------------------------------------------------------------
struct QuadtreeScene : public Scene
{
	QuadtreeScene();
	~QuadtreeScene();
	void Add(SceneNode* node) override;
	void Remove(SceneNode* node) override;
	void Clear() override;
	void ListNodes(SceneBatch& batch) override;
private:
	vector<SceneNode*> nodes;
	QuadTree quadtree;
};
