#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "DialogBox.h"
#include "TooltipController.h"
#include "FlowContainer.h"

//-----------------------------------------------------------------------------
struct PickItemDialogParams
{
	PickItemDialogParams() : event(nullptr), getTooltip(nullptr), parent(nullptr), multiple(0), sizeMin(300, 200), sizeMax(300, 512)
	{
	}

	vector<FlowItem*> items;
	DialogEvent event;
	TooltipController::Callback getTooltip;
	Control* parent;
	string text;
	Int2 sizeMin, sizeMax; // size.x is always taken from sizeMin for now
	int multiple;

	void AddItem(cstring itemText, int id, int group = 0, bool disabled = false);
	void AddSeparator(cstring itemText);
};

//-----------------------------------------------------------------------------
namespace layout
{
	struct PickItemDialog : public DialogBox
	{
		CustomButton close;
	};
}

//-----------------------------------------------------------------------------
class PickItemDialog : public DialogBox, public LayoutControl<layout::PickItemDialog>
{
public:
	using LayoutControl<layout::PickItemDialog>::layout;

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
	int GetSelectedId() const { return selected.empty() ? -1 : selected.front()->id; }
	vector<FlowItem*>& GetSelected()
	{
		return selected;
	}

	static PickItemDialog* Show(PickItemDialogParams& params);

private:
	enum Id
	{
		Cancel = GuiEvent_Custom
	};

	explicit PickItemDialog(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
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
