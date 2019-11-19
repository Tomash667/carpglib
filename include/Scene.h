#pragma once

//-----------------------------------------------------------------------------
#include "SceneNode.h"

//-----------------------------------------------------------------------------
struct Scene : public ObjectPoolProxy<Scene>
{
	vector<SceneNode*> nodes;

	void OnFree();
	void Add(SceneNode* node)
	{
		assert(node);
		node->scene = this;
		nodes.push_back(node);
	}
	void Remove(SceneNode* node)
	{
		assert(node);
		RemoveElement(nodes, node);
		node->Free();
	}
};
