/*
Basic window, have two modes:
1. normal - have header with text, can be moved
2. borderless - no header, takes full screen space, can't be moved
*/

#pragma once

//-----------------------------------------------------------------------------
#include "Container.h"
#include "Layout.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct Window : public Control
	{
		AreaLayout background;
		AreaLayout header;
		Font* font;
		Color fontColor;
		Int2 padding;
		int headerHeight;
	};
}

//-----------------------------------------------------------------------------
class Window : public Container, public LayoutControl<layout::Window>
{
public:
	Window(bool fullscreen = false, bool borderless = false);
	~Window();

	void Draw() override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;

	Int2 GetAreaSize() const { return Int2(area.Size()); }
	Control* GetEventProxy() const { return eventProxy; }
	MenuBar* GetMenu() const { return menu; }
	const string& GetText() const { return text; }
	bool IsBorderless() const { return borderless; }
	bool IsFullscreen() const { return fullscreen; }
	void SetAreaSize(const Int2& size);
	void SetEventProxy(Control* eventProxy) { this->eventProxy = eventProxy; }
	void SetMenu(MenuBar* menu);
	void SetText(Cstring s) { text = s.s; }

private:
	void CalculateArea();

	MenuBar* menu;
	Control* eventProxy;
	string text;
	Box2d bodyRect, headerRect, area;
	bool fullscreen, borderless;
};
