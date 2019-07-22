#include "EnginePch.h"
#include "EngineCore.h"
#include "Panel.h"

void Panel::Draw(ControlDrawData*)
{
	if(use_custom_color)
	{
		if(custom_color != Color::None)
			gui->DrawArea(custom_color, global_pos, size);
	}
	else
		gui->DrawArea(Box2d::Create(global_pos, size), layout->panel.background);

	Container::Draw();
}
