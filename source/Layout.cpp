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
	assert(this);
	layout::Control* c = types[type];
	assert(c);
	return c;
}


void AreaLayout::SetFromArea(const Rect* area)
{
	if(!tex->IsLoaded())
		app::resMgr->LoadInstant(tex);

	Int2 texSize = tex->GetSize();
	if(area)
	{
		size = area->Size();
		region = Box2d(*area) / Vec2((float)texSize.x, (float)texSize.y);
	}
	else
	{
		size = texSize;
		region = Box2d::Unit;
	}
}
