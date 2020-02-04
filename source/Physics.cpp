#include "EnginePch.h"
#include "EngineCore.h"
#include "Physics.h"

Physics* app::physics;

//=================================================================================================
Physics::Physics() : config(nullptr), dispatcher(nullptr), broadphase(nullptr), world(nullptr)
{
}

//=================================================================================================
Physics::~Physics()
{
	Reset();

	delete world;
	delete broadphase;
	delete dispatcher;
	delete config;
}

//=================================================================================================
void Physics::Init()
{
	config = new btDefaultCollisionConfiguration;
	dispatcher = new btCollisionDispatcher(config);
	broadphase = new btDbvtBroadphase;
	world = new btCollisionWorld(dispatcher, broadphase, config);
	Info("Engine: Bullet physics system created.");
}

//=================================================================================================
void Physics::Reset()
{
	btOverlappingPairCache* cache = broadphase->getOverlappingPairCache();
	btCollisionObjectArray& objs = world->getCollisionObjectArray();

	for(int i = 0, count = objs.size(); i < count; ++i)
	{
		btCollisionObject* obj = objs[i];
		btBroadphaseProxy* bp = obj->getBroadphaseHandle();
		if(bp)
		{
			cache->cleanProxyFromPairs(bp, dispatcher);
			broadphase->destroyProxy(bp, dispatcher);
		}
		delete obj;
	}

	objs.clear();
	broadphase->resetPool(dispatcher);
	DeleteElements(shapes);
}

//=================================================================================================
void Physics::UpdateAabb(btCollisionObject* cobj)
{
	btVector3 a_min, a_max;
	cobj->getCollisionShape()->getAabb(cobj->getWorldTransform(), a_min, a_max);
	broadphase->setAabb(cobj->getBroadphaseHandle(), a_min, a_max, dispatcher);
}
