#pragma once

//-----------------------------------------------------------------------------
#include "Gui.h"
#include "Texture.h"

//-----------------------------------------------------------------------------
class Control
{
	enum Flag
	{
		F_DOCKED = 1 << 0,
		F_ON_CHAR_HANDLER = 1 << 1
	};

public:
	explicit Control(bool isNew = false) : pos(0, 0), globalPos(0, 0), size(0, 0), parent(nullptr), visible(true), focus(false), mouseFocus(false),
		focusable(false), initialized(false), isNew(isNew), disabled(false), flags(0) {}
	virtual ~Control() {}

	static Int2 Center(const Int2& inSize) { return Int2((gui->wndSize.x - inSize.x) / 2, (gui->wndSize.y - inSize.y) / 2); }
	static Int2 Center(int w, int h) { return Int2((gui->wndSize.x - w) / 2, (gui->wndSize.y - h) / 2); }

	virtual void CalculateSize(int limitWidth) {}
	void Disable() { SetDisabled(true); }
	virtual void Dock(Control* c);
	virtual void Draw() {}
	void Enable() { SetDisabled(false); }
	virtual void Event(GuiEvent e) {}
	void GainFocus();
	Int2 GetCursorPos() const { return gui->cursorPos - pos; }
	const Int2& GetSize() const { return size; }
	void Hide() { Show(false); }
	void Initialize();
	bool IsDocked() const { return IsSet(flags, F_DOCKED); }
	bool IsInitialized() const { return initialized; }
	bool IsInside(const Int2& pt) const { return Rect::IsInside(pt, globalPos, size); }
	bool IsOnCharHandler() const { return IsSet(flags, F_ON_CHAR_HANDLER); }
	void LostFocus();
	virtual bool NeedCursor() const { return false; }
	virtual void SetDisabled(bool disabled) { this->disabled = disabled; }
	void SetDocked(bool docked);
	void SetFocus();
	void SetOnCharHandler(bool handle) { SetBitValue(flags, F_ON_CHAR_HANDLER, handle); }
	void SetPosition(const Int2& pos);
	void SetPosition(int x, int y) { SetPosition(Int2(x, y)); }
	void SetSize(const Int2& size);
	void SetSize(int w, int h) { SetSize(Int2(w, h)); }
	void Show() { Show(true); }
	void Show(bool visible);
	void SimpleDialog(cstring text) { gui->SimpleDialog(text, this); }
	void TakeFocus(bool pressed = false);
	virtual void Update(float dt) {}
	void UpdateControl(Control* ctrl, float dt);

	static Gui* gui;
	static Input* input;
	Int2 pos, globalPos, size;
	Control* parent;
	bool visible, focus,
		mouseFocus, // in Update it is set to true if Control can gain mouse focus, setting it to false mean that Control have taken focus
		focusable;

protected:
	bool initialized, isNew, disabled;

private:
	int flags;
};
