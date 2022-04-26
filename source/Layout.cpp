#include "Pch.h"
#include "Layout.h"

#include "ResourceManager.h"

Layout::Layout(Gui* gui) : gui(gui)
{
}

Layout::~Layout()
{
	DeleteElements(types);
}

layout::Control* Layout::Get(const type_info& type)
{
	layout::Control* c = types[type];
	assert(c);
	return c;
}


void AreaLayout::SetFromArea(const Rect* area)
{
	if(!tex->IsLoaded())
		app::resMgr->LoadInstant(tex);

	Int2 tex_size = tex->GetSize();
	if(area)
	{
		size = area->Size();
		region = Box2d(*area) / Vec2((float)tex_size.x, (float)tex_size.y);
	}
	else
	{
		size = tex_size;
		region = Box2d::Unit;
	}
}
