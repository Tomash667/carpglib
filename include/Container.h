#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
// Gui controls container
// new_mode - delete childs, different auto focus
class Container : public Control
{
	friend class Gui;
public:
	Container() : manual(false)
	{
		focusable = true;
	}
	~Container();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override;
	void SetDisabled(bool new_disabled) override;

	void Add(Control* ctrl);
	bool AnythingVisible() const;
	bool Empty() const { return ctrls.empty(); }
	vector<Control*>& GetControls() { return ctrls; }
	void Remove(Control* ctrl);
	Control* Top()
	{
		assert(!Empty());
		return ctrls.back();
	}
	Control* HitTest();
	void SetManual(bool manual) { this->manual = manual; }
	bool IsManual() const { return manual; }

protected:
	vector<Control*> ctrls;
	bool manual, inside_loop;
};
