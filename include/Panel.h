#pragma once

//-----------------------------------------------------------------------------
#include "Container.h"
#include "Layout.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct Panel : public Control
	{
		AreaLayout background;
	};
}

//-----------------------------------------------------------------------------
class Panel : public Container, public LayoutControl<layout::Panel>
{
public:
	Panel() : Container(true) {}
	void Draw() override;
};
