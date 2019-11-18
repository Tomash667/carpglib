#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneManager.h"

SceneManager* app::scene_mgr;

//=================================================================================================
SceneManager::SceneManager() : use_lighting(true), use_fog(true), use_normalmap(true), use_specularmap(true)
{
}
