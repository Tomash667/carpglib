#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct TabControl : public Control
	{
		AreaLayout background;
		AreaLayout line;
		AreaLayout button;
		AreaLayout buttonHover;
		AreaLayout buttonDown;
		AreaLayout close;
		AreaLayout closeHover;
		AreaLayout buttonPrev;
		AreaLayout buttonPrevHover;
		AreaLayout buttonNext;
		AreaLayout buttonNextHover;
		Font* font;
		Int2 padding;
		Int2 paddingActive;
		Color fontColor;
		Color fontColorHover;
		Color fontColorDown;
	};
}

//-----------------------------------------------------------------------------
class TabControl : public Control, public LayoutControl<layout::TabControl>
{
public:
	struct Tab
	{
		friend class TabControl;
	private:
		enum Mode
		{
			Up,
			Hover,
			Down
		};

		TabControl* parent;
		string text, id;
		Panel* panel;
		Mode mode;
		Int2 size;
		Box2d rect, closeRect;
		bool closeHover, haveChanges;

	public:
		void Close() { parent->Close(this); }
		bool GetHaveChanges() const { return haveChanges; }
		const string& GetId() const { return id; }
		TabControl* GetTabControl() const { return parent; }
		const string& GetText() const { return text; }
		bool IsSelected() const { return mode == Mode::Down; }
		void Select() { parent->Select(this); }
		void SetHaveChanges(bool haveChanges) { this->haveChanges = haveChanges; }
	};

	typedef delegate<bool(int, int)> Handler;

	enum Action
	{
		A_BEFORE_CHANGE,
		A_CHANGED,
		A_BEFORE_CLOSE
	};

	TabControl(bool ownPanels = true);
	~TabControl();

	void Dock(Control* c) override;
	void Draw() override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;

	Tab* AddTab(cstring id, cstring text, Panel* panel, bool select = true);
	void Clear();
	void Close(Tab* tab);
	Tab* Find(cstring id);
	Int2 GetAreaPos() const;
	Int2 GetAreaSize() const;
	Tab* GetCurrentTab() const { return selected; }
	Handler GetHandler() { return handler; }
	void Select(Tab* tab, bool scrollTo = true);
	void SetHandler(Handler handler) { this->handler = handler; }
	void ScrollTo(Tab* tab);

private:
	void Update(bool move, bool resize);
	void CalculateRect();
	void CalculateRect(Tab& tab, int offset);
	bool SelectInternal(Tab* tab);
	void CalculateTabOffsetMax();

	vector<Tab*> tabs;
	Tab* selected;
	Tab* hover;
	Box2d line;
	Handler handler;
	int height, totalWidth, tabOffset, tabOffsetMax, allowedSize;
	int arrowHover; // -1-prev, 0-none, 1-next
	bool ownPanels;
};
