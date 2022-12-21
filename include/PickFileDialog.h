#pragma once

//-----------------------------------------------------------------------------
#include "GuiDialog.h"
#include "Layout.h"

//-----------------------------------------------------------------------------
class PickFileDialogItem;
struct PickFileDialogOptions;

//-----------------------------------------------------------------------------
enum class PreviewType
{
	None,
	Text,
	Image
};

//-----------------------------------------------------------------------------
namespace layout
{
	struct PickFileDialog : public Control
	{
		AreaLayout box;
	};
}

//-----------------------------------------------------------------------------
class PickFileDialog : public GuiDialog, public LayoutControl<layout::PickFileDialog>
{
public:
	typedef delegate<void(PickFileDialog*)> Handler;
	using LayoutControl<layout::PickFileDialog>::layout;

	static void Show(const PickFileDialogOptions& options);
	void Draw() override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;

	bool GetResult() const { return result; }
	const string& GetPath() const { assert(result); return resultPath; }
	const string& GetFilename() const { assert(result); return resultFilename; }

private:
	enum EventId
	{
		SelectItem = GuiEvent_Custom,
		Cancel
	};

	struct Filter
	{
		string text;
		vector<string> exts;
	};

	PickFileDialog();
	~PickFileDialog();

	void Setup(const PickFileDialogOptions& options);
	void LoadDir(bool keepSelection);
	void ParseFilters(const string& str);
	bool HandleChangeExtension(int action, int index);
	bool HandleListBoxEvent(int action, int index);
	void PickItem();
	void PickDir(PickFileDialogItem* item);
	void CancelPick();
	void SetupPreview();

	static PickFileDialog* self;
	ListBox* listBox, *listExtensions;
	TextBox* tbPath, *tbFilename, *tbPreview;
	Button* btSelect, *btCancel;
	DrawBox* drawBox;
	Label* labelPreview;
	string rootDir, activeDir, resultFilename, resultPath;
	vector<Filter> filters;
	Filter* activeFilter;
	Handler handler;
	Texture* texDir;
	std::map<string, PreviewType> previewTypes;
	bool result, preview;
};

//-----------------------------------------------------------------------------
struct PickFileDialogOptions
{
	PickFileDialog::Handler handler;
	string title, filters, rootDir, startPath;
	bool preview;

	void Show() { PickFileDialog::Show(*this); }
};
