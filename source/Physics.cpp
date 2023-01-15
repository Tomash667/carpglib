#include "Pch.h"
#include "Physics.h"

#include "SimpleMesh.h"
#include "VertexData.h"

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

	for(btBvhTriangleMeshShape* trimesh : trimeshes)
	{
		delete reinterpret_cast<SimpleMesh*>(trimesh->getUserPointer());
		delete trimesh->getMeshInterface();
		delete trimesh;
	}
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
}

//=================================================================================================
void Physics::UpdateAabb(btCollisionObject* cobj)
{
	btVector3 aabbMin, aabbMax;
	cobj->getCollisionShape()->getAabb(cobj->getWorldTransform(), aabbMin, aabbMax);
	broadphase->setAabb(cobj->getBroadphaseHandle(), aabbMin, aabbMax, dispatcher);
}

//=================================================================================================
btBvhTriangleMeshShape* Physics::CreateTrimeshShape(VertexData* vd)
{
	SimpleMesh* simpleMesh = new SimpleMesh;
	simpleMesh->vd = vd;

	btIndexedMesh mesh;
	mesh.m_numTriangles = vd->faces.size();
	mesh.m_triangleIndexBase = reinterpret_cast<byte*>(vd->faces.data());
	mesh.m_triangleIndexStride = sizeof(Face);
	mesh.m_numVertices = vd->verts.size();
	mesh.m_vertexBase = reinterpret_cast<byte*>(vd->verts.data());
	mesh.m_vertexStride = sizeof(Vec3);

	btTriangleIndexVertexArray* shapeData = new btTriangleIndexVertexArray();
	shapeData->addIndexedMesh(mesh, PHY_SHORT);
	btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(shapeData, true);
	shape->setUserPointer(simpleMesh);
	trimeshes.push_back(shape);
	return shape;
}
