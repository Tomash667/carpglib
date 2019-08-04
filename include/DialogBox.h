#pragma once

//-----------------------------------------------------------------------------
#include "Button.h"
#include "Checkbox.h"
#include "Layout.h"

//-----------------------------------------------------------------------------
enum DialogButtonId
{
	BUTTON_OK,
	BUTTON_YES,
	BUTTON_NO,
	BUTTON_CANCEL,
	BUTTON_CHECKED,
	BUTTON_UNCHECKED,
	BUTTON_CUSTOM
};

//-----------------------------------------------------------------------------
namespace layout
{
	struct DialogBox : public Control
	{
		AreaLayout background;
		AreaLayout box;
		Font* font;
	};
}

//-----------------------------------------------------------------------------
class DialogBox : public Control, public LayoutControl<layout::DialogBox>
{
public:
	explicit DialogBox(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	virtual void Setup(const Int2& text_size) {}

	void CloseDialog() { gui->CloseDialog(this); }

	string name, text;
	GUI_DialogType type;
	DialogEvent event;
	DialogOrder order;
	vector<Button> bts;
	int result;
	bool pause, need_delete;

protected:
	void DrawPanel(bool background = true);
};

//-----------------------------------------------------------------------------
class DialogWithCheckbox : public DialogBox
{
public:
	explicit DialogWithCheckbox(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	CheckBox checkbox;
};

//-----------------------------------------------------------------------------
class DialogWithImage : public DialogBox
{
public:
	explicit DialogWithImage(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Setup(const Int2& text_size) override;

	const Int2& GetImageSize() const { return img_size; }

private:
	Texture* img;
	Int2 img_size, img_pos;
	Rect text_rect;
};
