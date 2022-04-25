#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct Scrollbar : public Control
	{
		AreaLayout tex, tex2;
	};
}

//-----------------------------------------------------------------------------
class Scrollbar : public Control, public LayoutControl<layout::Scrollbar>
{
public:
	explicit Scrollbar(bool hscrollbar = false);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;

	void LostFocus();
	// porusza scrollbar myszk¹, zwraca czy cokolwiek siê zmieni³o
	bool ApplyMouseWheel();
	void SetValue(float p) { offset = float(total - part)*p; }
	float GetValue() const { return offset / float(total - part); }
	void UpdateTotal(int new_total);
	void UpdateOffset(float change);
	bool IsRequired() const { return total > part; }

	int total, part, change;
	float offset;
	Int2 click_pt;
	bool clicked, hscrollbar, manual_change;
};
