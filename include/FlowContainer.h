#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "Scrollbar.h"
#include "Button.h"

//-----------------------------------------------------------------------------
// for backward compatibility, still used in CreateCharacterPanel
struct OldFlowItem
{
	bool section;
	int group, id, y;
	float part;

	OldFlowItem(int group, int id, int y) : section(true), group(group), id(id), y(y)
	{
	}

	OldFlowItem(int group, int id, int min, int max, int value, int y) : section(false), group(group), id(id), y(y)
	{
		part = float(value - min) / (max - min);
	}
};

//-----------------------------------------------------------------------------
struct FlowItem
{
	enum Type
	{
		Item,
		Section,
		Button
	};

	static ObjectPool<FlowItem> Pool;
	Type type;
	string text;
	int group, id, tex_id;
	Int2 pos, size;
	Button::State state;

	// set for section
	void Set(cstring text);
	// set for item
	void Set(cstring text, int group, int id);
	// set for button
	void Set(int group, int id, int tex_id, bool disabled = false);
};

//-----------------------------------------------------------------------------
typedef delegate<void(int, int)> ButtonEvent;

//-----------------------------------------------------------------------------
namespace layout
{
	struct FlowContainer : public Control
	{
		AreaLayout box;
		AreaLayout selection;
		Font* font;
		Font* font_section;
	};
}

//-----------------------------------------------------------------------------
class FlowContainer : public Control, public LayoutControl<layout::FlowContainer>
{
public:
	FlowContainer();
	~FlowContainer();

	void Update(float dt);
	void Draw(ControlDrawData* cdd = nullptr);
	FlowItem* Add();
	void Clear();
	// set group & index only if there is selection
	void GetSelected(int& group, int& id);
	void UpdateSize(const Int2& pos, const Int2& size, bool visible);
	void UpdatePos(const Int2& parent_pos);
	void Reposition();
	FlowItem* Find(int group, int id);
	void ResetScrollbar() { scroll.offset = 0.f; }
	void SetItems(vector<FlowItem*>& _items);
	int GetHeight() const { return scroll.total; }
	void UpdateText(FlowItem* item, cstring text, bool batch = false);
	void UpdateText(int group, int id, cstring text, bool batch = false) { UpdateText(Find(group, id), text, batch); }
	void UpdateText();

	vector<FlowItem*> items;
	ButtonEvent on_button;
	CustomButton* button_tex;
	Int2 button_size;
	bool word_warp, allow_select;
	VoidF on_select;
	FlowItem* selected;

private:
	void UpdateScrollbar(int new_size);

	int group, id;
	Scrollbar scroll;
	bool batch_changes;
};
