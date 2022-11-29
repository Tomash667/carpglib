#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
// Gui controls container
// isNew - delete childs, different auto focus
class Container : public Control
{
	friend class Gui;
public:
	explicit Container(bool isNew = false) : Control(isNew), autoFocus(false), focusTop(false), dontFocus(false)
	{
		focusable = true;
	}
	~Container();

	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override;
	void SetDisabled(bool disabled) override;

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

	bool autoFocus, focusTop, dontFocus;

protected:
	vector<Control*> ctrls;
	bool insideLoop;
};
