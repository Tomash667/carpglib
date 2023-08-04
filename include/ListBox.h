#pragma once

//-----------------------------------------------------------------------------
#include "Scrollbar.h"
#include "GuiElement.h"
#include "Layout.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct ListBox : public Control
	{
		AreaLayout box;
		AreaLayout selection;
		AreaLayout hover;
		Texture* downArrow;
		Font* font;
		Color fontColor[2];
		int padding, autoPadding, border;
	};
}

//-----------------------------------------------------------------------------
class ListBox : public Control, public OnCharHandler, public LayoutControl<layout::ListBox>
{
public:
	enum Action
	{
		A_BEFORE_CHANGE_INDEX,
		A_INDEX_CHANGED,
		A_BEFORE_MENU_SHOW,
		A_MENU,
		A_DOUBLE_CLICK
	};

	typedef delegate<bool(int, int)> Handler;

	ListBox(bool isNew = false);
	~ListBox();

	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void OnChar(char c) override;

	void Add(GuiElement* e, bool select = false);
	void Add(cstring text, int value = 0, Texture* tex = nullptr) { Add(new DefaultGuiElement(text, value, tex)); }
	void Sort();
	void ScrollTo(int index, bool center = false);
	GuiElement* Find(int value) const;
	int FindIndex(int value) const;
	int FindIndex(GuiElement* e) const;
	void Select(int index, bool sendEvent = false);
	void Select(delegate<bool(GuiElement*)> pred);
	void SelectByValue(int value, bool send_event = false);
	void ForceSelect(int index);
	void Insert(GuiElement* e, int index);
	void Remove(int index);
	void Remove(GuiElement* e) { Remove(FindIndex(e)); }
	void Remove() { if(selected != -1) Remove(selected); }
	void Reset();

	int GetIndex() const { return selected; }
	int GetHover() const { return hover; }
	GuiElement* GetItem() const { return selected == -1 ? nullptr : items[selected]; }
	template<typename T>
	T* GetItemCast() const { return static_cast<T*>(GetItem()); }
	int GetItemHeight() const { return itemHeight; }
	const Int2& GetForceImageSize() const { return forceImgSize; }
	vector<GuiElement*>& GetItems() { return items; }
	template<typename T>
	vector<T*>& GetItemsCast()
	{
		static_assert(std::is_convertible<T*, GuiElement*>::value);
		return reinterpret_cast<vector<T*>&>(items);
	}
	uint GetCount() const { return items.size(); }

	bool IsCollapsed() { return collapsed; }
	bool IsEmpty() const { return items.empty(); }

	void SetCollapsed(bool collapsed)
	{
		assert(!initialized);
		this->collapsed = collapsed;
	}
	void SetIndex(int index)
	{
		assert(index >= -1 && index < (int)items.size());
		selected = index;
	}
	void SetItemHeight(int height)
	{
		assert(height >= -1);
		itemHeight = height;
	}
	void SetForceImageSize(const Int2& size)
	{
		assert(size.x >= 0 && size.y >= 0);
		forceImgSize = size;
	}

	MenuList* menu;
	MenuStrip* menuStrip;
	DialogEvent eventHandler;
	Handler eventHandler2;
	int textFlags;

private:
	int PosToIndex(int y);
	void OnSelect(int index);
	bool ChangeIndexEvent(int index, bool force, bool scrollTo);
	void UpdateScrollbarVisibility();
	void CalculateItemHeight(GuiElement* e);
	int CalculateItemsHeight();

	Scrollbar scrollbar;
	vector<GuiElement*> items;
	int selected; // index of selected item or -1, default -1
	int hover; // index of hover item or -1
	int itemHeight; // height of item, default 20, -1 is auto
	Int2 realSize;
	Int2 forceImgSize; // forced image size, Int2(0,0) if not forced, default Int2(0,0)
	bool collapsed, requireScrollbar;
};
