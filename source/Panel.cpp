#include "Pch.h"
#include "Panel.h"

void Panel::Draw()
{
	gui->DrawArea(Box2d::Create(globalPos, size), layout->background);

	Container::Draw();
}
