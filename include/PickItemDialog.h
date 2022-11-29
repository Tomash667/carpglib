#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "DialogBox.h"
#include "TooltipController.h"
#include "FlowContainer.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct PickItemDialog : public DialogBox
	{
		CustomButton close;
	};
}

//-----------------------------------------------------------------------------
struct PickItemDialogParams
{
	PickItemDialogParams() : event(nullptr), getTooltip(nullptr), parent(nullptr), multiple(0), sizeMin(300, 200), sizeMax(300, 512)
	{
	}

	Control* parent;
	DialogEvent event;
	TooltipController::Callback getTooltip;
	vector<FlowItem*> items;
	string text;
	Int2 sizeMin, sizeMax; // size.x is always taken from sizeMin for now
	int multiple;

	void AddItem(cstring itemText, int group, int id, bool disabled = false);
	void AddSeparator(cstring itemText);
};

//-----------------------------------------------------------------------------
class PickItemDialog : public DialogBox, public LayoutControl<layout::PickItemDialog>
{
public:
	using LayoutControl<layout::PickItemDialog>::layout;

	static PickItemDialog* Show(PickItemDialogParams& params);
	void GetSelected(int& group, int& id) const
	{
		if(!selected.empty())
		{
			group = selected[0]->group;
			id = selected[0]->id;
		}
		else
		{
			group = -1;
			id = -1;
		}
	}
	vector<FlowItem*>& GetSelected()
	{
		return selected;
	}

private:
	enum Id
	{
		Cancel = GuiEvent_Custom
	};

	explicit PickItemDialog(const DialogInfo& info);

	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Create(PickItemDialogParams& params);
	void OnSelect();

	static PickItemDialog* self;
	FlowContainer flow;
	TooltipController::Callback getTooltip;
	Button btClose;
	TooltipController tooltip;
	vector<FlowItem*> selected;
	int multiple;
};
