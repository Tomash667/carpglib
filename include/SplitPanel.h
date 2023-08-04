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

	void Draw() override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;

	Panel* GetPanel1() { return panel1; }
	Panel* GetPanel2() { return panel2; }
	uint GetSplitterSize() { return splitterSize; }
	bool IsHorizontal() const { return horizontal; }
	void SetPanel1(Panel* panel);
	void SetPanel2(Panel* panel);
	void SetSplitterSize(uint splitterSize);

	uint minSize1, minSize2;
	bool allowMove;

private:
	void Update(GuiEvent e, bool resize, bool move);

	Panel* panel1, *panel2;
	Rect split, splitGlobal;
	FixedPanel fixed;
	uint splitterSize;
	bool horizontal;
};
