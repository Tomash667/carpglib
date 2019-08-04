#pragma once

//-----------------------------------------------------------------------------
#include "MenuStrip.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct MenuBar : public Control
	{
		AreaLayout background;
		AreaLayout button;
		AreaLayout button_hover;
		AreaLayout button_down;
		Font* font;
		Int2 padding;
		Int2 item_padding;
		Color font_color;
		Color font_color_hover;
		Color font_color_down;
	};
}

//-----------------------------------------------------------------------------
class MenuBar : public Control, public LayoutControl<layout::MenuBar>
{
	struct Item
	{
		enum Mode
		{
			Up,
			Hover,
			Down
		};

		string text;
		Box2d rect;
		int index;
		Mode mode;
		vector<SimpleMenuCtor> items;
		MenuStrip* menu;

		Item() : menu(nullptr) {}
		~Item() { delete menu; }
	};

public:
	MenuBar();
	~MenuBar();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void AddMenu(cstring text, std::initializer_list<SimpleMenuCtor> const & items);
	void ChangeMenu(int offset);
	void SetHandler(MenuStrip::Handler _handler) { handler = _handler; }

private:
	void Update(bool move, bool resize);
	void OnCloseMenuStrip();
	void EnsureMenu(Item* item);

	vector<Item*> items;
	Item* selected;
	MenuStrip::Handler handler;
	Box2d rect;
};
