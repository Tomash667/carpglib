#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "DialogBox.h"
#include "TooltipController.h"
#include "FlowContainer.h"

//-----------------------------------------------------------------------------
struct PickItemDialogParams
{
	PickItemDialogParams() : event(nullptr), get_tooltip(nullptr), parent(nullptr), multiple(0), size_min(300, 200), size_max(300, 512)
	{
	}

	vector<FlowItem*> items;
	int multiple;
	DialogEvent event;
	TooltipController::Callback get_tooltip;
	Control* parent;
	string text;
	Int2 size_min, size_max; // size.x is always taken from size_min for now

	void AddItem(cstring item_text, int group, int id, bool disabled = false);
	void AddSeparator(cstring item_text);
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
	vector<FlowItem*>& GetSelected()
	{
		return selected;
	}

	static PickItemDialog* Show(PickItemDialogParams& params);

	static PickItemDialog* self;

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

	FlowContainer flow;
	TooltipController::Callback get_tooltip;
	int multiple;
	Button btClose;
	TooltipController tooltip;
	vector<FlowItem*> selected;
};
