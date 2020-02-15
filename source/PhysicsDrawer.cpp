#include "EnginePch.h"
#include "EngineCore.h"
#include "PhysicsDrawer.h"
#include "Physics.h"
#include "BasicShader.h"
#include "Render.h"

static BasicShader* shader;

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
	shader->BeginBatch();

	btCollisionWorld* world = app::physics->GetWorld();
	world->setDebugDrawer(this);
	world->debugDrawWorld();

	shader->EndBatch();
}

void PhysicsDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	shader->AddLine(ToVec3(from), ToVec3(to), 0.01f, Color(255, 0, 128));
}
