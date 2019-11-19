#include "EnginePch.h"
#include "EngineCore.h"
#include "Scene.h"
#include "SceneNode.h"

//=================================================================================================
void Scene::OnFree()
{
	SceneNode::Free(nodes);
}
