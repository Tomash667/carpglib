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
		AreaLayout buttonHover;
		AreaLayout buttonDown;
		Font* font;
		Int2 padding;
		Int2 itemPadding;
		Color fontColor;
		Color fontColorHover;
		Color fontColorDown;
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

	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void AddMenu(cstring text, std::initializer_list<SimpleMenuCtor> const & items);
	void ChangeMenu(int offset);
	void SetHandler(MenuStrip::Handler handler) { this->handler = handler; }

private:
	void Update(bool move, bool resize);
	void OnCloseMenuStrip();
	void EnsureMenu(Item* item);

	vector<Item*> items;
	Item* selected;
	MenuStrip::Handler handler;
	Box2d rect;
};
