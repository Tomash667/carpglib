#include "EnginePch.h"
#include "EngineCore.h"
#include "Layout.h"
#include "Gui.h"
#include "ResourceManager.h"
#include "Label.h"

Layout::Layout(Gui* gui) : gui(gui)
{
}

Layout::~Layout()
{
	delete label;
}

void Layout::LoadDefault()
{
	ResourceManager& res_mgr = ResourceManager::Get();
	Font* def_font = gui->default_font;
	Font* font_big = gui->fBig;

	panel.background = AreaLayout(Color(245, 246, 247), Color(0xA0, 0xA0, 0xA0));

	window.background = AreaLayout(Color(0xAB, 0xAB, 0xAB), Color::Black);
	window.header = AreaLayout(Color(128, 128, 128), Color::Black);
	window.font = font_big;
	window.font_color = Color::Black;
	window.header_height = font_big->height + 4;
	window.padding = Int2(2, 2);

	menubar.background = AreaLayout(Color(245, 246, 247), Color(0xA0, 0xA0, 0xA0));
	menubar.button = AreaLayout(Color(245, 246, 247));
	menubar.button_hover = AreaLayout(Color(213, 231, 248), Color(122, 177, 232));
	menubar.button_down = AreaLayout(Color(184, 216, 249), Color(98, 163, 229));
	menubar.font = def_font;
	menubar.padding = Int2(4, 4);
	menubar.item_padding = Int2(10, 2);
	menubar.font_color = Color::Black;
	menubar.font_color_hover = Color::Black;
	menubar.font_color_down = Color::Black;

	menustrip.background = AreaLayout(Color(245, 246, 247), Color(0xA0, 0xA0, 0xA0));
	menustrip.button_hover = AreaLayout(Color(51, 153, 255));
	menustrip.font = def_font;
	menustrip.padding = Int2(2, 2);
	menustrip.item_padding = Int2(2, 2);
	menustrip.font_color = Color::Black;
	menustrip.font_color_hover = Color::Black;
	menustrip.font_color_disabled = Color(128, 128, 128);

	tabctrl.background = AreaLayout(Color(245, 246, 247));
	tabctrl.line = AreaLayout(Color(245, 246, 247), Color(0xA0, 0xA0, 0xA0));
	tabctrl.button = AreaLayout(Color(238, 238, 238), Color(172, 172, 172));
	tabctrl.button_hover = AreaLayout(Color(238, 238, 238), Color(142, 176, 200));
	tabctrl.button_down = AreaLayout(Color(233, 243, 252), Color(126, 180, 234));
	tabctrl.font = def_font;
	tabctrl.font_color = Color::Black;
	tabctrl.font_color_hover = Color::Black;
	tabctrl.font_color_down = Color::Black;
	tabctrl.padding = Int2(8, 4);
	tabctrl.padding_active = Int2(8, 8);
	tabctrl.close = AreaLayout(res_mgr.Load<Texture>("close_small.png"));
	tabctrl.close.size = Int2(12, 12);
	tabctrl.close_hover = AreaLayout(tabctrl.close.tex, Color(51, 153, 255));
	tabctrl.close_hover.size = Int2(12, 12);
	tabctrl.button_prev = AreaLayout(res_mgr.Load<Texture>("tabctrl_arrow.png"), Rect(0, 0, 12, 16));
	tabctrl.button_prev_hover = AreaLayout(tabctrl.button_prev.tex, Rect(0, 0, 12, 16), Color(51, 153, 255));
	tabctrl.button_next = AreaLayout(tabctrl.button_prev.tex, Rect(16, 0, 28, 16));
	tabctrl.button_next_hover = AreaLayout(tabctrl.button_prev.tex, Rect(16, 0, 28, 16), Color(51, 153, 255));

	tree_view.background = AreaLayout(res_mgr.Load<Texture>("box.png"), 8, 32);
	tree_view.button = AreaLayout(res_mgr.Load<Texture>("treeview.png"), Rect(0, 0, 16, 16));
	tree_view.button_hover = AreaLayout(tree_view.button.tex, Rect(16, 0, 32, 16));
	tree_view.button_down = AreaLayout(tree_view.button.tex, Rect(32, 0, 48, 16));
	tree_view.button_down_hover = AreaLayout(tree_view.button.tex, Rect(48, 0, 64, 16));
	tree_view.font = def_font;
	tree_view.font_color = Color::Black;
	tree_view.selected = AreaLayout(Color(51, 153, 255));
	tree_view.level_offset = 16;
	tree_view.text_box_background = res_mgr.Load<Texture>("box2.png");
	tree_view.drag_n_drop = res_mgr.Load<Texture>("drag_n_drop.png");

	split_panel.background = AreaLayout(Color(0xAB, 0xAB, 0xAB), Color(0xA0, 0xA0, 0xA0));
	split_panel.padding = Int2(0, 0);
	split_panel.horizontal = AreaLayout(res_mgr.Load<Texture>("split_panel.png"), Rect(3, 2, 4, 5));
	split_panel.vertical = AreaLayout(split_panel.horizontal.tex, Rect(11, 3, 14, 4));

	label = new LabelLayout;
	label->font = def_font;
	label->color = Color::Black;
	label->padding = Int2(0, 0);
	label->align = DTF_LEFT;

	check_box_group.background = AreaLayout(res_mgr.Load<Texture>("box.png"), 8, 32);
	check_box_group.box = AreaLayout(res_mgr.Load<Texture>("checkbox.png"), Rect(0, 0, 16, 16));
	check_box_group.checked = AreaLayout(check_box_group.box.tex, Rect(16, 0, 32, 16));
	check_box_group.font = def_font;
	check_box_group.font_color = Color::Black;
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
	if(tex->IsLoaded())
	{
		Int2 tex_size = tex->GetSize();
		if(area)
		{
			size = area->Size();
			region = Box2d(*area) / Vec2((float)tex_size.x, (float)tex_size.y);
		}
		else
			size = tex_size;
	}
	else
	{
		Optional<Rect> area_backing(area);
		ResourceManager::Get().AddTask(this, [area_backing](TaskData& data)
		{
			AreaLayout* area = reinterpret_cast<AreaLayout*>(data.ptr);
			area->SetFromArea(area_backing);
		});
	}
}
