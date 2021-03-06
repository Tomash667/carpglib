#pragma once

//-----------------------------------------------------------------------------
#include "Panel.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct SplitPanel : public Control
	{
		AreaLayout background;
		AreaLayout horizontal;
		AreaLayout vertical;
		Int2 padding;
	};
}

//-----------------------------------------------------------------------------
class SplitPanel : public Control, public LayoutControl<layout::SplitPanel>
{
public:
	enum class FixedPanel
	{
		None,
		Panel1,
		Panel2
	};

	SplitPanel();
	~SplitPanel();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;

	Panel* GetPanel1() { return panel1; }
	Panel* GetPanel2() { return panel2; }
	uint GetSplitterSize() { return splitter_size; }
	bool IsHorizontal() const { return horizontal; }
	void SetPanel1(Panel* panel);
	void SetPanel2(Panel* panel);
	void SetSplitterSize(uint splitter_size);

	uint min_size1, min_size2;
	bool allow_move;

private:
	void Update(GuiEvent e, bool resize, bool move);

	Panel* panel1, *panel2;
	Rect split, split_global;
	FixedPanel fixed;
	int splitter_size;
	bool horizontal;
};
