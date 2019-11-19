#pragma once

//-----------------------------------------------------------------------------
struct Scene : public ObjectPoolProxy<Scene>
{
	vector<SceneNode*> nodes;

	void OnFree();
};
