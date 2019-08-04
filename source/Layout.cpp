#include "EnginePch.h"
#include "EngineCore.h"
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


Box2d AreaLayout::CalculateRegion(const Int2& pos, const Int2& region)
{
	Box2d box;
	box.v1.x = (float)pos.x + (region.x - size.x) / 2;
	box.v1.y = (float)pos.y + (region.y - size.y) / 2;
	box.v2.x = box.v1.x + size.x;
	box.v2.y = box.v1.y + size.y;
	return box;
}

void AreaLayout::SetFromArea(const Rect* area)
{
	if(!tex->IsLoaded())
		ResourceManager::Get().LoadInstant(tex);

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
