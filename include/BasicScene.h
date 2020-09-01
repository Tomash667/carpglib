#pragma once

//-----------------------------------------------------------------------------
#include "Scene.h"

//-----------------------------------------------------------------------------
struct BasicScene : public Scene
{
	void Add(SceneNode* node) override;
	void Remove(SceneNode* node) override;
	void Clear() override;
	void ListNodes(SceneBatch& batch) override;
private:
	vector<SceneNode*> nodes;
};
