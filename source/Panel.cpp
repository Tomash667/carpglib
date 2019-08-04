#include "EnginePch.h"
#include "EngineCore.h"
#include "Panel.h"

void Panel::Draw(ControlDrawData*)
{
	gui->DrawArea(Box2d::Create(global_pos, size), layout->background);

	Container::Draw();
}
