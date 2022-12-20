#include "Pch.h"
#include "PickFileDialog.h"

#include "Button.h"
#include "DrawBox.h"
#include "File.h"
#include "Input.h"
#include "Label.h"
#include "ListBox.h"
#include "Overlay.h"
#include "ResourceManager.h"
#include "TextBox.h"

class PickFileDialogItem : public GuiElement
{
public:
	cstring ToString()
	{
		return filename.c_str();
	}

	string filename, path;
	bool isDir;
};

bool PickFileDialogItemSort(const PickFileDialogItem* i1, const PickFileDialogItem* i2)
{
	if(i1->isDir == i2->isDir)
	{
		int r = _stricmp(i1->filename.c_str(), i2->filename.c_str());
		if(r == 0)
			return i1->filename < i2->filename;
		else
			return r < 0;
	}
	else
		return i1->isDir;
}

PickFileDialog* PickFileDialog::self;

PickFileDialog::PickFileDialog()
{
	SetAreaSize(Int2(640, 480));

	listBox = new ListBox(true);
	listBox->eventHandler2 = ListBox::Handler(this, &PickFileDialog::HandleListBoxEvent);
	listBox->SetSize(Int2(640 - 4, 480 - 100));
	listBox->SetPosition(Int2(2, 34));
	Add(listBox);

	listExtensions = new ListBox(true);
	listExtensions->eventHandler2 = ListBox::Handler(this, &PickFileDialog::HandleChangeExtension);
	listExtensions->SetSize(Int2(640 - 406, 30));
	listExtensions->SetPosition(Int2(404, 480 - 64));
	listExtensions->SetCollapsed(true);
	Add(listExtensions);

	tbPath = new TextBox(true);
	tbPath->SetReadonly(true);
	tbPath->SetSize(Int2(640 - 4, 30));
	tbPath->SetPosition(Int2(2, 2));
	Add(tbPath);

	tbFilename = new TextBox(true);
	tbFilename->SetSize(Int2(400, 30));
	tbFilename->SetPosition(Int2(2, 480 - 64));
	Add(tbFilename);

	btSelect = new Button;
	btSelect->id = SelectItem;
	btSelect->text = "Open";
	btSelect->SetSize(Int2(100, 30));
	btSelect->SetPosition(Int2(640 - 212, 480 - 32));
	Add(btSelect);

	btCancel = new Button;
	btCancel->id = Cancel;
	btCancel->text = "Cancel";
	btCancel->SetSize(Int2(100, 30));
	btCancel->SetPosition(Int2(640 - 102, 480 - 32));
	Add(btCancel);

	drawBox = new DrawBox;
	drawBox->SetSize(Int2(240 - 6, 480 - 100));
	drawBox->SetPosition(Int2(404, 34));
	Add(drawBox);

	tbPreview = new TextBox(true);
	tbPreview->SetReadonly(true);
	tbPreview->SetMultiline(true);
	tbPreview->SetSize(Int2(240 - 6, 480 - 100));
	tbPreview->SetPosition(Int2(404, 34));
	Add(tbPreview);

	labelPreview = new Label("Preview not available", false);
	labelPreview->SetSize(Int2(240 - 6, 480 - 100));
	labelPreview->SetPosition(Int2(404, 34));
	Add(labelPreview);

	texDir = app::resMgr->Load<Texture>("dir.png");

	previewTypes["txt"] = PreviewType::Text;
	previewTypes["bmp"] = PreviewType::Image;
	previewTypes["jpg"] = PreviewType::Image;
	previewTypes["tga"] = PreviewType::Image;
	previewTypes["png"] = PreviewType::Image;
	previewTypes["dds"] = PreviewType::Image;
	previewTypes["ppm"] = PreviewType::Image;
	previewTypes["dib"] = PreviewType::Image;
	previewTypes["hdr"] = PreviewType::Image;
	previewTypes["pfm"] = PreviewType::Image;
}

PickFileDialog::~PickFileDialog()
{
}

void PickFileDialog::Show(const PickFileDialogOptions& options)
{
	if(!self)
	{
		self = new PickFileDialog;
		gui->RegisterControl(self);
	}
	self->Setup(options);
	gui->GetOverlay()->ShowDialog(self);
}

void PickFileDialog::Setup(const PickFileDialogOptions& options)
{
	SetText(options.title);
	rootDir = options.rootDir;
	handler = options.handler;
	preview = options.preview;
	if(preview)
		listBox->SetSize(Int2(400, 480 - 100));
	else
		listBox->SetSize(Int2(640 - 4, 480 - 100));
	activeDir = rootDir;
	ParseFilters(options.filters);
	LoadDir(false);
	SetupPreview();
}

void PickFileDialog::Draw()
{
	Window::Draw();

	if(labelPreview->visible)
		gui->DrawArea(Box2d::Create(labelPreview->globalPos, labelPreview->size), layout->box);
}

void PickFileDialog::Event(GuiEvent e)
{
	switch(e)
	{
	case SelectItem:
		PickItem();
		break;
	case Cancel:
		CancelPick();
		break;
	default:
		GuiDialog::Event(e);
		break;
	}
}

void PickFileDialog::Update(float dt)
{
	if(input->PressedRelease(Key::Escape))
		CancelPick();
	if(listBox->focus)
	{
		if(input->PressedRelease(Key::Enter))
			PickItem();
		else if(input->PressedRelease(Key::Backspace) && !listBox->IsEmpty())
		{
			auto item = listBox->GetItemsCast<PickFileDialogItem>()[0];
			if(item->filename == "..")
				PickDir(item);
		}
	}
	else if(tbFilename->focus && input->PressedRelease(Key::Enter))
	{
		string filename = Trimmed(tbFilename->GetText());
		if(!filename.empty())
		{
			bool ok = false;
			auto& items = listBox->GetItemsCast<PickFileDialogItem>();
			uint index = 0;
			for(auto item : items)
			{
				if(item->filename == filename)
				{
					if(item->isDir)
						tbFilename->Reset();
					listBox->Select(index);
					ok = true;
					PickItem();
					break;
				}
				++index;
			}

			if(!ok)
				SimpleDialog(Format("Can't file file or directory '%s'.", filename.c_str()));
		}
	}

	Window::Update(dt);
}

string GetParentDir(const string& path)
{
	std::size_t pos = path.find_last_of('/');
	string part = path.substr(0, pos);
	return part;
}

string GetExt(const string& filename)
{
	std::size_t pos = filename.find_last_of('.');
	if(pos == string::npos)
		return string();
	string ext = filename.substr(pos + 1);
	return ext;
}

void PickFileDialog::LoadDir(bool keepSelection)
{
	string oldFilename;
	auto selected = listBox->GetItemCast<PickFileDialogItem>();
	if(selected)
		oldFilename = selected->filename;
	tbPath->SetText(Format("%s/", activeDir.c_str()));
	listBox->Reset();

	// add parent dir
	if(activeDir != rootDir)
	{
		auto item = new PickFileDialogItem;
		item->filename = "..";
		item->path = GetParentDir(activeDir);
		item->isDir = true;
		item->tex = texDir;
		listBox->Add(item);
	}

	// add all files/dirs matching filter
	io::FindFiles(Format("%s/*.*", activeDir.c_str()), [this](const io::FileInfo& info)
	{
		if(info.isDir)
		{
			auto item = new PickFileDialogItem;
			item->filename = info.filename;
			item->path = Format("%s/%s", activeDir.c_str(), item->filename.c_str());
			item->isDir = true;
			item->tex = texDir;
			listBox->Add(item);
		}
		else
		{
			string filename = info.filename;
			string ext = GetExt(filename);

			bool ok = false;
			if(activeFilter->exts.empty())
				ok = true;
			else
			{
				for(auto& filter : activeFilter->exts)
				{
					if(filter == ext)
					{
						ok = true;
						break;
					}
				}
			}

			if(ok)
			{
				auto item = new PickFileDialogItem;
				item->filename = filename;
				item->path = Format("%s/%s", activeDir.c_str(), item->filename.c_str());
				item->isDir = false;
				listBox->Add(item);
			}
		}
		return true;
	});

	// sort items
	auto& items = listBox->GetItemsCast<PickFileDialogItem>();
	std::sort(items.begin(), items.end(), PickFileDialogItemSort);

	// keep old selected item if it exists
	if(keepSelection && !oldFilename.empty())
	{
		uint index = 0;
		for(auto item : items)
		{
			if(item->filename == oldFilename)
			{
				listBox->Select(index);
				break;
			}
			++index;
		}
	}
}

void SplitText(const string& str, vector<string>& splitted, char separator, bool ignoreEmpty)
{
	uint pos = 0;
	string text;

	while(true)
	{
		uint pos2 = str.find_first_of(separator, pos);
		if(pos2 == string::npos)
		{
			text = str.substr(pos);
			if(!text.empty() || !ignoreEmpty)
				splitted.push_back(text);
			break;
		}

		text = str.substr(pos, pos2 - pos);
		if(!text.empty() || !ignoreEmpty)
			splitted.push_back(text);

		pos = pos2 + 1;
	}
}

void PickFileDialog::ParseFilters(const string& str)
{
	filters.clear();

	uint pos = 0;
	string text, ext;

	while(true)
	{
		uint pos2 = str.find_first_of(';', pos);
		if(pos2 == string::npos)
			break;
		text = str.substr(pos, pos2 - pos);
		++pos2;
		pos = str.find_first_of(';', pos2);
		if(pos == string::npos)
			ext = str.substr(pos2);
		else
			ext = str.substr(pos2, pos - pos2);
		Filter f;
		f.text = std::move(text);
		if(ext != "*")
			SplitText(ext, f.exts, ',', true);
		filters.push_back(std::move(f));
		if(pos == string::npos)
			break;
		++pos;
	}

	if(filters.empty())
	{
		Filter f;
		f.text = "All files";
		filters.push_back(f);
	}

	activeFilter = &filters[0];

	listExtensions->Reset();
	for(uint i = 0; i < filters.size(); ++i)
		listExtensions->Add(filters[i].text.c_str(), i);
	listExtensions->Select(0);
}

bool PickFileDialog::HandleChangeExtension(int action, int index)
{
	if(action == ListBox::A_INDEX_CHANGED)
	{
		activeFilter = &filters[index];
		LoadDir(true);
	}

	return true;
}

bool PickFileDialog::HandleListBoxEvent(int action, int index)
{
	switch(action)
	{
	case ListBox::A_INDEX_CHANGED:
		{
			auto item = listBox->GetItemCast<PickFileDialogItem>();
			if(!item->isDir)
				tbFilename->SetText(item->filename.c_str());
			SetupPreview();
		}
		break;
	case ListBox::A_DOUBLE_CLICK:
		PickItem();
		break;
	}

	return true;
}

void PickFileDialog::PickItem()
{
	auto item = listBox->GetItemCast<PickFileDialogItem>();
	if(!item)
		return;
	if(item->isDir)
		PickDir(item);
	else
	{
		result = true;
		resultFilename = item->filename;
		resultPath = item->path;
		Close();
		if(handler)
			handler(this);
	}
}

void PickFileDialog::PickDir(PickFileDialogItem* item)
{
	if(item->filename == "..")
	{
		string currentDir = io::FilenameFromPath(activeDir);
		activeDir = item->path;
		LoadDir(false);

		// select old parent dir
		auto& items = listBox->GetItemsCast<PickFileDialogItem>();
		uint index = 0;
		for(auto item : items)
		{
			if(item->filename == currentDir)
			{
				listBox->Select(index);
				break;
			}
			++index;
		}
	}
	else
	{
		activeDir = item->path;
		LoadDir(false);
	}
}

void PickFileDialog::CancelPick()
{
	result = false;
	Close();
	if(handler)
		handler(this);
}

void PickFileDialog::SetupPreview()
{
	tbPreview->visible = false;
	drawBox->visible = false;
	labelPreview->visible = false;

	if(!preview)
		return;

	PreviewType type;
	auto item = listBox->GetItemCast<PickFileDialogItem>();
	if(!item || item->isDir)
		type = PreviewType::None;
	else
	{
		string ext = GetExt(item->filename);
		auto it = previewTypes.find(ext);
		if(it == previewTypes.end())
			type = PreviewType::None;
		else
			type = it->second;
	}

	switch(type)
	{
	case PreviewType::None:
		labelPreview->visible = true;
		break;
	case PreviewType::Text:
		{
			tbPreview->visible = true;
			tbPreview->Reset();
			LocalString preview;
			io::LoadFileToString(item->path.c_str(), preview.get_ref(), 1024 * 1024 * 1024); // max 1MB
			tbPreview->SetText(preview);
		}
		break;
	case PreviewType::Image:
		{
			drawBox->visible = true;
			drawBox->SetTexture(app::resMgr->LoadInstant<Texture>(item->path));
		}
		break;
	}
}
