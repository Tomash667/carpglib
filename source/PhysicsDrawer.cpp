#include "Pch.h"
#include "PhysicsDrawer.h"

#include "BasicShader.h"
#include "Physics.h"
#include "Render.h"

static BasicShader* shader;

//=================================================================================================
void PhysicsDrawer::Draw(Camera& camera)
{
	if(!enabled)
		return;

	if(!shader)
	{
		shader = new BasicShader;
		app::render->RegisterShader(shader);
	}

	shader->Prepare(camera);
	app::render->SetDepthState(Render::DEPTH_NO);

	btCollisionWorld* world = app::physics->GetWorld();
	world->setDebugDrawer(this);
	world->debugDrawWorld();

	shader->Draw();
}

//=================================================================================================
void PhysicsDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	shader->DrawLine(ToVec3(from), ToVec3(to), 0.01f, Color(color.getX(), color.getY(), color.getZ()));
}
