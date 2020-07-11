#pragma once

//-----------------------------------------------------------------------------
#include "SceneCallback.h"
#include <LinearMath\btIDebugDraw.h>

//-----------------------------------------------------------------------------
struct PhysicsDrawer : public SceneCallback, public btIDebugDraw
{
	PhysicsDrawer();
	void Draw(Camera& camera) override;

	void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
	void drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor) override
	{
		drawLine(from, to, fromColor);
	}
	void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {}
	void reportErrorWarning(const char* warningString) override {}
	void draw3dText(const btVector3& location, const char* textString) override {}
	void setDebugMode(int debugMode) override {}
	int getDebugMode() const override { return DBG_DrawWireframe; }

	BasicShader* shader;
	bool enabled;
};
