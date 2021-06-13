#pragma once

//-----------------------------------------------------------------------------
#include "Gui.h"
#include "Texture.h"

//-----------------------------------------------------------------------------
struct ControlDrawData
{
	Box2d* clipping;
};

//-----------------------------------------------------------------------------
inline bool PointInRect(const Int2& pt, int left, int top, int right, int bottom)
{
	return pt.x >= left && pt.y >= top && pt.x < right && pt.y < bottom;
}

//-----------------------------------------------------------------------------
inline bool PointInRect(const Int2& pt, const Int2& rpos, const Int2& rsize)
{
	return pt.x >= rpos.x && pt.y >= rpos.y && pt.x < rpos.x + rsize.x && pt.y < rpos.y + rsize.y;
}

//-----------------------------------------------------------------------------
class Control
{
public:
	enum Flag
	{
		F_DOCKED = 1 << 0,
		F_ON_CHAR_HANDLER = 1 << 1
	};

	explicit Control(bool is_new = false) : pos(0, 0), global_pos(0, 0), size(0, 0), parent(nullptr), visible(true), focus(false), mouse_focus(false),
		focusable(false), initialized(false), is_new(is_new), disabled(false), flags(0) {}
	virtual ~Control() {}

	static Gui* gui;
	static Input* input;
	Int2 pos, global_pos, size;
	Control* parent;
	bool visible, focus,
		mouse_focus, // in Update it is set to true if Control can gain mouse focus, setting it to false mean that Control have taken focus
		focusable;

protected:
	bool initialized, is_new, disabled;

private:
	int flags;

public:
	// virtual
	virtual void CalculateSize(int limit_width) {}
	virtual void Dock(Control* c);
	virtual void Draw(ControlDrawData* cdd = nullptr) {}
	virtual void Event(GuiEvent e) {}
	virtual bool NeedCursor() const { return false; }
	virtual void SetDisabled(bool new_disabled) { disabled = new_disabled; }
	virtual void Update(float dt) {}

	Int2 GetCursorPos() const
	{
		return gui->cursor_pos - pos;
	}

	bool IsInside(const Int2& pt) const
	{
		return pt.x >= global_pos.x && pt.y >= global_pos.y && pt.x < global_pos.x + size.x && pt.y < global_pos.y + size.y;
	}

	static Int2 Center(const Int2& in_size) { return Int2((gui->wnd_size.x - in_size.x) / 2, (gui->wnd_size.y - in_size.y) / 2); }
	static Int2 Center(int w, int h) { return Int2((gui->wnd_size.x - w) / 2, (gui->wnd_size.y - h) / 2); }

	void SimpleDialog(cstring text)
	{
		gui->SimpleDialog(text, this);
	}

	void GainFocus()
	{
		if(!focus)
		{
			focus = true;
			Event(GuiEvent_GainFocus);
		}
	}

	void LostFocus()
	{
		if(focus)
		{
			focus = false;
			Event(GuiEvent_LostFocus);
		}
	}

	void Show()
	{
		if(!visible)
		{
			visible = true;
			Event(GuiEvent_Show);
		}
	}

	void Show(bool visible)
	{
		if(this->visible != visible)
		{
			this->visible = visible;
			Event(visible ? GuiEvent_Show : GuiEvent_Hide);
		}
	}

	void Hide()
	{
		if(visible)
		{
			visible = false;
			Event(GuiEvent_Hide);
		}
	}

	void Disable() { SetDisabled(true); }
	void Enable() { SetDisabled(false); }
	const Int2& GetSize() const { return size; }
	void Initialize();
	void SetSize(const Int2& size);
	void SetSize(int w, int h) { SetSize(Int2(w, h)); }
	void SetPosition(const Int2& pos);
	void SetPosition(int x, int y) { SetPosition(Int2(x, y)); }
	void SetDocked(bool docked);
	void TakeFocus(bool pressed = false);
	void SetFocus();
	void UpdateControl(Control* ctrl, float dt);

	bool IsOnCharHandler() const { return IsSet(flags, F_ON_CHAR_HANDLER); }
	bool IsDocked() const { return IsSet(flags, F_DOCKED); }
	bool IsInitialized() const { return initialized; }

	void SetOnCharHandler(bool handle) { SetBitValue(flags, F_ON_CHAR_HANDLER, handle); }
};
