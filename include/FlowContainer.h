#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "Scrollbar.h"
#include "Button.h"

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
		Font* fontSection;
	};
}

//-----------------------------------------------------------------------------
class FlowContainer : public Control, public LayoutControl<layout::FlowContainer>
{
public:
	FlowContainer();
	~FlowContainer();

	FlowItem* Add();
	void Clear();
	void Draw();
	FlowItem* Find(int group, int id);
	int GetHeight() const { return scroll.total; }
	void GetSelected(int& group, int& id);
	void Reposition();
	void ResetScrollbar() { scroll.offset = 0.f; }
	void SetItems(vector<FlowItem*>& items);
	void Update(float dt);
	void UpdatePos(const Int2& parentPos);
	void UpdateSize(const Int2& pos, const Int2& size, bool visible);
	void UpdateText();
	void UpdateText(FlowItem* item, cstring text, bool batch = false);
	void UpdateText(int group, int id, cstring text, bool batch = false) { UpdateText(Find(group, id), text, batch); }

	vector<FlowItem*> items;
	ButtonEvent onButton;
	VoidF onSelect;
	CustomButton* buttonTex;
	Int2 buttonSize;
	bool wordWrap, allowSelect;
	FlowItem* selected;

private:
	void UpdateScrollbar(int size);

	int group, id;
	Scrollbar scroll;
	bool batchChanges;
};
