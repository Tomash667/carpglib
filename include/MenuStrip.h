#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
struct SimpleMenuCtor
{
	cstring text;
	int id;
};

//-----------------------------------------------------------------------------
namespace layout
{
	struct MenuStrip : public Control
	{
		AreaLayout background;
		AreaLayout buttonHover;
		Font* font;
		Int2 padding;
		Int2 itemPadding;
		Color fontColor;
		Color fontColorHover;
		Color fontColorDisabled;
	};
}

//-----------------------------------------------------------------------------
class MenuStrip : public Control, public LayoutControl<layout::MenuStrip>, public OnCharHandler
{
public:
	struct Item
	{
		friend MenuStrip;

		int GetAction() const { return action; }
		int GetIndex() const { return index; }

		bool IsEnabled() const { return enabled; }

		void SetEnabled(bool enabled) { this->enabled = enabled; }

	private:
		string text;
		int index, action;
		bool hover, enabled;
	};

	typedef delegate<void(int)> Handler;
	typedef delegate<void()> OnCloseHandler;

	MenuStrip(vector<SimpleMenuCtor>& items, int minWIdth = 0);
	MenuStrip(vector<GuiElement*>& items, int minWIdth = 0);
	~MenuStrip();

	void Draw() override;
	void OnChar(char c) override;
	void Update(float dt) override;

	// internal! call only from Overlay -> Move to better place
	void ShowAt(const Int2& pos);
	void ShowMenu() { ShowMenu(gui->cursorPos); }
	void ShowMenu(const Int2& pos);
	void OnClose()
	{
		if(onCloseHandler)
			onCloseHandler();
	}
	Item* FindItem(int action);
	void SetHandler(Handler handler) { this->handler = handler; }
	void SetOnCloseHandler(OnCloseHandler parentMenuBar) { this->onCloseHandler = parentMenuBar; }
	void SetOwner(MenuBar* parentMenuBar, int menuBarIndex)
	{
		this->parentMenuBar = parentMenuBar;
		this->menuBarIndex = menuBarIndex;
	}
	void SetSelectedIndex(int index);
	bool IsOpen();

private:
	void CalculateWidth(int minWIdth);
	void ChangeIndex(int dir);
	void UpdateMouse();
	void UpdateKeyboard();

	Handler handler;
	OnCloseHandler onCloseHandler;
	vector<Item> items;
	Item* selected;
	MenuBar* parentMenuBar;
	int menuBarIndex;
};
