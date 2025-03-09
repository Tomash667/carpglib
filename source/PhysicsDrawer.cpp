#include "Pch.h"
#include "PhysicsDrawer.h"

#include "BasicShader.h"
#include "Physics.h"
#include "Render.h"

//=================================================================================================
PhysicsDrawer::PhysicsDrawer() : shader(app::render->GetShader<BasicShader>())
{
}

//=================================================================================================
void PhysicsDrawer::Draw(Camera& camera)
{
	shader->Prepare(camera);
	app::render->SetDepthState(Render::DEPTH_NO);

	btCollisionWorld* world = app::physics->GetWorld();
	world->setDebugDrawer(this);
	world->debugDrawWorld();

	shader->Draw();
}

//=================================================================================================
void PhysicsDrawer::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
	Vec3 p = ToVec3(PointOnB);
	shader->DrawLine(Vec3(p.x - 0.25f, p.y, p.z), Vec3(p.x + 0.25f, p.y, p.z), 0.01f, Color(color.getX(), color.getY(), color.getZ()));
	shader->DrawLine(Vec3(p.x, p.y - 0.25f, p.z), Vec3(p.x, p.y + 0.25f, p.z), 0.01f, Color(color.getX(), color.getY(), color.getZ()));
	shader->DrawLine(Vec3(p.x, p.y, p.z - 0.25f), Vec3(p.x, p.y, p.z + 0.25f), 0.01f, Color(color.getX(), color.getY(), color.getZ()));
}

//=================================================================================================
void PhysicsDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	shader->DrawLine(ToVec3(from), ToVec3(to), 0.01f, Color(color.getX(), color.getY(), color.getZ()));
}
