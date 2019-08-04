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
		Color font_color;
		Int2 padding;
		int header_height;
	};
}

//-----------------------------------------------------------------------------
class Window : public Container, public LayoutControl<layout::Window>
{
public:
	Window(bool fullscreen = false, bool borderless = false);
	~Window();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;

	Int2 GetAreaSize() const { return Int2(area.Size()); }
	Control* GetEventProxy() const { return event_proxy; }
	MenuBar* GetMenu() const { return menu; }
	const string& GetText() const { return text; }
	bool IsBorderless() const { return borderless; }
	bool IsFullscreen() const { return fullscreen; }
	void SetAreaSize(const Int2& size);
	void SetEventProxy(Control* _event_proxy) { event_proxy = _event_proxy; }
	void SetMenu(MenuBar* menu);
	void SetText(Cstring s) { text = s.s; }

private:
	void CalculateArea();

	MenuBar* menu;
	Control* event_proxy;
	string text;
	Box2d body_rect, header_rect, area;
	bool fullscreen, borderless;
};
