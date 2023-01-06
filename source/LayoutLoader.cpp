#include "Pch.h"
#include "LayoutLoader.h"

#include "Button.h"
#include "CheckBox.h"
#include "CheckBoxGroup.h"
#include "FlowContainer.h"
#include "Grid.h"
#include "InputTextBox.h"
#include "Label.h"
#include "ListBox.h"
#include "MenuBar.h"
#include "MenuList.h"
#include "MenuStrip.h"
#include "Notifications.h"
#include "Overlay.h"
#include "Panel.h"
#include "PickFileDialog.h"
#include "PickItemDialog.h"
#include "ResourceManager.h"
#include "Scrollbar.h"
#include "Slider.h"
#include "SplitPanel.h"
#include "TabControl.h"
#include "TextBox.h"
#include "TooltipController.h"
#include "TreeView.h"
#include "Window.h"

enum KeywordGroup
{
	G_TOP,
	G_AREA,
	G_FONT,
	G_AREA_MODE
};

enum TopKeyword
{
	TK_REGISTER_FONT,
	TK_FONT,
	TK_CONTROL
};

enum AreaKeyword
{
	AK_MODE,
	AK_IMAGE,
	AK_CORNER,
	AK_SIZE,
	AK_COLOR,
	AK_RECT,
	AK_BORDER_COLOR,
	AK_BACKGROUND_COLOR
};

enum FontKeyword
{
	FK_NAME,
	FK_SIZE,
	FK_WEIGHT,
	FK_OUTLINE
};

//=================================================================================================
LayoutLoader::~LayoutLoader()
{
	DeleteElements(controls);
}

//=================================================================================================
Layout* LayoutLoader::LoadFromFile(cstring inputPath)
{
	path = inputPath;

	if(!initialized)
	{
		t.SetFlags(Tokenizer::F_MULTI_KEYWORDS);
		RegisterKeywords();
		RegisterControls();
		initialized = true;
	}

	layout = new Layout(gui);

	try
	{
		t.FromFile(path);
		while(t.Next())
		{
			TopKeyword top = t.MustGetKeywordId<TopKeyword>(G_TOP);
			t.Next();

			const string& name = t.MustGetString();
			switch(top)
			{
			case TK_REGISTER_FONT:
				gui->AddFont(name.c_str());
				break;
			case TK_FONT:
				ParseFont(name);
				break;
			case TK_CONTROL:
				ParseControl(name);
				break;
			}
		}
	}
	catch(Tokenizer::Exception& ex)
	{
		Error("Failed to load layout '%s': %s", path.c_str(), ex.ToString());
		delete layout;
		layout = nullptr;
	}

	return layout;
}

//=================================================================================================
LayoutLoader::Control* LayoutLoader::FindControl(const string& name)
{
	auto it = controls.find(name);
	if(it == controls.end())
		return nullptr;
	return it->second;
}

//=================================================================================================
Font* LayoutLoader::FindFont(const string& name)
{
	auto it = fonts.find(name);
	if(it == fonts.end())
		return nullptr;
	return it->second;
}

//=================================================================================================
void LayoutLoader::ParseFont(const string& name)
{
	if(FindFont(name))
		t.Throw("Font '%s' already exists.", name.c_str());

	string n = name, fontName;
	int size = -1, weight = 5, outline = 0;

	t.Next();
	t.AssertSymbol('{');
	t.Next();
	while(!t.IsSymbol('}'))
	{
		FontKeyword keyword = t.MustGetKeywordId<FontKeyword>(G_FONT);
		t.Next();
		switch(keyword)
		{
		case FK_NAME:
			fontName = t.MustGetString();
			break;
		case FK_SIZE:
			size = t.MustGetInt();
			if(size < 1)
				t.Throw("Invalid font size '%d'.", size);
			break;
		case FK_WEIGHT:
			weight = t.MustGetInt();
			if(weight < 1 || weight > 9)
				t.Throw("Invalid font weight '%d'.", weight);
			break;
		case FK_OUTLINE:
			outline = t.MustGetInt();
			break;
		}
		t.Next();
	}

	if(fontName.empty() || size < 1)
		t.Throw("Font name or size not set.");

	Font* font = gui->GetFont(fontName.c_str(), size, weight, outline);
	fonts[n] = font;
}

//=================================================================================================
void LayoutLoader::ParseControl(const string& name)
{
	Control* c = FindControl(name);
	if(!c)
		t.Throw("Missing control '%s'.", name.c_str());
	if(c->used)
		t.Throw("Control '%s' is already declared.", name.c_str());
	c->used = true;

	byte* data = new byte[c->size];
	layout->types[c->type] = reinterpret_cast<layout::Control*>(data);

	t.Next();
	t.AssertSymbol('{');
	t.Next();
	while(!t.IsSymbol('}'))
	{
		const string& entryName = t.MustGetItemKeyword();
		Entry* entry = c->FindEntry(entryName);
		if(!entry)
			t.Throw("Missing control '%s' entry '%s'.", c->name.c_str(), entryName.c_str());
		t.Next();

		switch(entry->type)
		{
		case Entry::AreaLayout:
			{
				AreaLayout& area = entry->GetValue<AreaLayout>(data);
				t.AssertSymbol('{');
				t.Next();
				t.AssertKeyword(AK_MODE, G_AREA);
				t.Next();
				area.mode = t.MustGetKeywordId<AreaLayout::Mode>(G_AREA_MODE);
				switch(area.mode)
				{
				case AreaLayout::Mode::Color:
					area.color = Color::White;
					break;
				case AreaLayout::Mode::BorderColor:
					area.color = Color::White;
					area.borderColor = Color::Black;
					area.width = 1;
					break;
				case AreaLayout::Mode::Image:
					area.tex = nullptr;
					area.color = Color::White;
					area.backgroundColor = Color::None;
					area.size = Int2::Zero;
					break;
				case AreaLayout::Mode::Item:
					area.tex = nullptr;
					area.color = Color::White;
					area.size = Int2::Zero;
					break;
				}
				t.Next();
				Rect rect = Rect::Zero;
				while(!t.IsSymbol('}'))
				{
					AreaKeyword key = t.MustGetKeywordId<AreaKeyword>(G_AREA);
					t.Next();
					switch(key)
					{
					case AK_MODE:
						t.Throw("Mode already set.");
						break;
					case AK_IMAGE:
						{
							if(area.mode != AreaLayout::Mode::Image && area.mode != AreaLayout::Mode::Item)
								t.Throw("This area layout don't support 'image' entry.");
							const string& imgName = t.MustGetString();
							area.tex = app::resMgr->Load<Texture>(imgName);
							t.Next();
						}
						break;
					case AK_CORNER:
						if(area.mode != AreaLayout::Mode::Item)
							t.Throw("This area layout don't support 'corner' entry.");
						area.size.x = t.MustGetInt();
						if(area.size.x < 1)
							t.Throw("Invalid 'corner' value '%d'.", area.size.x);
						t.Next();
						break;
					case AK_SIZE:
						if(area.mode != AreaLayout::Mode::Item)
							t.Throw("This area layout don't support 'size' entry.");
						area.size.y = t.MustGetInt();
						if(area.size.y < 1)
							t.Throw("Invalid 'size' value '%d'.", area.size.y);
						t.Next();
						break;
					case AK_COLOR:
						t.Parse(area.color);
						break;
					case AK_RECT:
						if(area.mode != AreaLayout::Mode::Image)
							t.Throw("This area layout don't support 'rect' entry.");
						t.Parse(rect);
						break;
					case AK_BORDER_COLOR:
						if(area.mode != AreaLayout::Mode::BorderColor)
							t.Throw("This area layout don't support 'borderColor' entry.");
						t.Parse(area.borderColor);
						break;
					case AK_BACKGROUND_COLOR:
						if(area.mode != AreaLayout::Mode::Image)
							t.Throw("This area layout don't support 'backgroundColor' entry.");
						t.Parse(area.backgroundColor);
						break;
					}
				}
				if(area.mode == AreaLayout::Mode::Image || area.mode == AreaLayout::Mode::Item)
				{
					if(!area.tex)
						t.Throw("Missing area image.");
				}
				if(area.mode == AreaLayout::Mode::Item)
				{
					if(area.size.x == 0 || area.size.y == 0)
						t.Throw("Missing area size.");
				}
				if(area.mode == AreaLayout::Mode::Image)
					area.SetFromArea(rect != Rect::Zero ? &rect : nullptr);
				t.Next();
			}
			break;
		case Entry::Font:
			{
				const string& fontName = t.MustGetString();
				Font* font = FindFont(fontName);
				if(!font)
					t.Throw("Missing font '%s'.", fontName.c_str());
				entry->GetValue<Font*>(data) = font;
				t.Next();
			}
			break;
		case Entry::Image:
			{
				const string& imgName = t.MustGetString();
				entry->GetValue<Texture*>(data) = app::resMgr->Load<Texture>(imgName);
				t.Next();
			}
			break;
		case Entry::Color:
			t.Parse(entry->GetValue<Color>(data));
			break;
		case Entry::Int2:
			t.Parse(entry->GetValue<Int2>(data));
			break;
		case Entry::Int:
			entry->GetValue<int>(data) = t.MustGetInt();
			t.Next();
			break;
		case Entry::Bool:
			entry->GetValue<bool>(data) = t.MustGetBool();
			t.Next();
			break;
		}
	}
}

//=================================================================================================
void LayoutLoader::RegisterKeywords()
{
	t.AddKeywords(G_TOP, {
		{"registerFont",TK_REGISTER_FONT},
		{"font", TK_FONT},
		{"control", TK_CONTROL}
		});

	t.AddKeywords(G_AREA, {
		{"mode", AK_MODE},
		{"image",AK_IMAGE},
		{"corner",AK_CORNER},
		{"size",AK_SIZE},
		{"color",AK_COLOR},
		{"rect",AK_RECT},
		{"borderColor",AK_BORDER_COLOR},
		{"backgroundColor",AK_BACKGROUND_COLOR}
		});

	t.AddKeywords(G_FONT, {
		{"name",FK_NAME},
		{"size",FK_SIZE},
		{"weight",FK_WEIGHT},
		{"outline",FK_OUTLINE}
		});

	t.AddKeywords<AreaLayout::Mode>(G_AREA_MODE, {
		{"COLOR",AreaLayout::Mode::Color},
		{"BORDER_COLOR",AreaLayout::Mode::BorderColor},
		{"IMAGE",AreaLayout::Mode::Image},
		{"ITEM",AreaLayout::Mode::Item}
		});
}

//=================================================================================================
void LayoutLoader::RegisterControls()
{
	Control* c;

	c = AddControl<layout::Button>("Button");
	c->AddEntry("tex", Entry::AreaLayout, offsetof(layout::Button, tex[Button::NONE]));
	c->AddEntry("texHover", Entry::AreaLayout, offsetof(layout::Button, tex[Button::HOVER]));
	c->AddEntry("texDown", Entry::AreaLayout, offsetof(layout::Button, tex[Button::DOWN]));
	c->AddEntry("texDisabled", Entry::AreaLayout, offsetof(layout::Button, tex[Button::DISABLED]));
	c->AddEntry("font", Entry::Font, offsetof(layout::Button, font));
	c->AddEntry("fontColor", Entry::Color, offsetof(layout::Button, fontColor[Button::NONE]));
	c->AddEntry("fontColorHover", Entry::Color, offsetof(layout::Button, fontColor[Button::HOVER]));
	c->AddEntry("fontColorDown", Entry::Color, offsetof(layout::Button, fontColor[Button::DOWN]));
	c->AddEntry("fontColorDisabled", Entry::Color, offsetof(layout::Button, fontColor[Button::DISABLED]));
	c->AddEntry("padding", Entry::Int, offsetof(layout::Button, padding));
	c->AddEntry("outline", Entry::Bool, offsetof(layout::Button, outline));

	c = AddControl<layout::CheckBox>("CheckBox");
	c->AddEntry("tex", Entry::AreaLayout, offsetof(layout::CheckBox, tex[CheckBox::NONE]));
	c->AddEntry("texHover", Entry::AreaLayout, offsetof(layout::CheckBox, tex[CheckBox::HOVER]));
	c->AddEntry("texDown", Entry::AreaLayout, offsetof(layout::CheckBox, tex[CheckBox::DOWN]));
	c->AddEntry("texDisabled", Entry::AreaLayout, offsetof(layout::CheckBox, tex[CheckBox::DISABLED]));
	c->AddEntry("font", Entry::Font, offsetof(layout::CheckBox, font));
	c->AddEntry("tick", Entry::AreaLayout, offsetof(layout::CheckBox, tick));

	c = AddControl<layout::CheckBoxGroup>("CheckBoxGroup");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::CheckBoxGroup, background));
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::CheckBoxGroup, box));
	c->AddEntry("checked", Entry::AreaLayout, offsetof(layout::CheckBoxGroup, checked));
	c->AddEntry("font", Entry::Font, offsetof(layout::CheckBoxGroup, font));
	c->AddEntry("fontColor", Entry::Color, offsetof(layout::CheckBoxGroup, fontColor));

	c = AddControl<layout::DialogBox>("DialogBox");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::DialogBox, background));
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::DialogBox, box));
	c->AddEntry("font", Entry::Font, offsetof(layout::DialogBox, font));

	c = AddControl<layout::FlowContainer>("FlowContainer");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::FlowContainer, box));
	c->AddEntry("hover", Entry::AreaLayout, offsetof(layout::FlowContainer, hover));
	c->AddEntry("selection", Entry::AreaLayout, offsetof(layout::FlowContainer, selection));
	c->AddEntry("font", Entry::Font, offsetof(layout::FlowContainer, font));
	c->AddEntry("fontSection", Entry::Font, offsetof(layout::FlowContainer, fontSection));
	c->AddEntry("border", Entry::Int, offsetof(layout::FlowContainer, border));
	c->AddEntry("padding", Entry::Int, offsetof(layout::FlowContainer, padding));

	c = AddControl<layout::Grid>("Grid");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::Grid, box));
	c->AddEntry("selection", Entry::AreaLayout, offsetof(layout::Grid, selection));
	c->AddEntry("font", Entry::Font, offsetof(layout::Grid, font));
	c->AddEntry("border", Entry::Int, offsetof(layout::Grid, border));

	c = AddControl<layout::Gui>("Gui");
	c->AddEntry("cursor", Entry::Image, offsetof(layout::Gui, cursor[CURSOR_NORMAL]));
	c->AddEntry("cursorHover", Entry::Image, offsetof(layout::Gui, cursor[CURSOR_HOVER]));
	c->AddEntry("cursorText", Entry::Image, offsetof(layout::Gui, cursor[CURSOR_TEXT]));

	c = AddControl<layout::InputTextBox>("InputTextBox");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::InputTextBox, box));
	c->AddEntry("font", Entry::Font, offsetof(layout::InputTextBox, font));

	c = AddControl<layout::Label>("Label");
	c->AddEntry("font", Entry::Font, offsetof(layout::Label, font));
	c->AddEntry("color", Entry::Color, offsetof(layout::Label, color));

	c = AddControl<layout::ListBox>("ListBox");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::ListBox, box));
	c->AddEntry("selection", Entry::AreaLayout, offsetof(layout::ListBox, selection));
	c->AddEntry("hover", Entry::AreaLayout, offsetof(layout::ListBox, hover));
	c->AddEntry("downArrow", Entry::Image, offsetof(layout::ListBox, downArrow));
	c->AddEntry("font", Entry::Font, offsetof(layout::ListBox, font));
	c->AddEntry("padding", Entry::Int, offsetof(layout::ListBox, padding));
	c->AddEntry("autoPadding", Entry::Int, offsetof(layout::ListBox, autoPadding));
	c->AddEntry("border", Entry::Int, offsetof(layout::ListBox, border));

	c = AddControl<layout::MenuBar>("MenuBar");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::MenuBar, background));
	c->AddEntry("button", Entry::AreaLayout, offsetof(layout::MenuBar, button));
	c->AddEntry("buttonHover", Entry::AreaLayout, offsetof(layout::MenuBar, buttonHover));
	c->AddEntry("buttonDown", Entry::AreaLayout, offsetof(layout::MenuBar, buttonDown));
	c->AddEntry("font", Entry::Font, offsetof(layout::MenuBar, font));
	c->AddEntry("padding", Entry::Int2, offsetof(layout::MenuBar, padding));
	c->AddEntry("itemPadding", Entry::Int2, offsetof(layout::MenuBar, itemPadding));
	c->AddEntry("fontColor", Entry::Color, offsetof(layout::MenuBar, fontColor));
	c->AddEntry("fontColorHover", Entry::Color, offsetof(layout::MenuBar, fontColorHover));
	c->AddEntry("fontColorDown", Entry::Color, offsetof(layout::MenuBar, fontColorDown));

	c = AddControl<layout::MenuList>("MenuList");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::MenuList, box));
	c->AddEntry("selection", Entry::AreaLayout, offsetof(layout::MenuList, selection));
	c->AddEntry("font", Entry::Font, offsetof(layout::MenuList, font));
	c->AddEntry("border", Entry::Int, offsetof(layout::MenuList, border));
	c->AddEntry("padding", Entry::Int, offsetof(layout::MenuList, padding));
	c->AddEntry("itemHeight", Entry::Int, offsetof(layout::MenuList, itemHeight));

	c = AddControl<layout::MenuStrip>("MenuStrip");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::MenuStrip, background));
	c->AddEntry("buttonHover", Entry::AreaLayout, offsetof(layout::MenuStrip, buttonHover));
	c->AddEntry("font", Entry::Font, offsetof(layout::MenuStrip, font));
	c->AddEntry("padding", Entry::Int2, offsetof(layout::MenuStrip, padding));
	c->AddEntry("itemPadding", Entry::Int2, offsetof(layout::MenuStrip, itemPadding));
	c->AddEntry("fontColor", Entry::Color, offsetof(layout::MenuStrip, fontColor));
	c->AddEntry("fontColorHover", Entry::Color, offsetof(layout::MenuStrip, fontColorHover));
	c->AddEntry("fontColorDisabled", Entry::Color, offsetof(layout::MenuStrip, fontColorDisabled));

	c = AddControl<layout::Notifications>("Notifications");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::Notifications, box));
	c->AddEntry("button", Entry::AreaLayout, offsetof(layout::Notifications, button));
	c->AddEntry("font", Entry::Font, offsetof(layout::Notifications, font));
	c->AddEntry("accept", Entry::Image, offsetof(layout::Notifications, accept));
	c->AddEntry("cancel", Entry::Image, offsetof(layout::Notifications, cancel));

	c = AddControl<layout::Overlay>("Overlay");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::Overlay, background));

	c = AddControl<layout::Panel>("Panel");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::Panel, background));

	c = AddControl<layout::PickFileDialog>("PickFileDialog");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::PickFileDialog, box));

	c = AddControl<layout::PickItemDialog>("PickItemDialog");
	c->AddEntry("closeButton", Entry::AreaLayout, offsetof(layout::PickItemDialog, close.tex[Button::NONE]));
	c->AddEntry("closeButtonHover", Entry::AreaLayout, offsetof(layout::PickItemDialog, close.tex[Button::HOVER]));
	c->AddEntry("closeButtonDown", Entry::AreaLayout, offsetof(layout::PickItemDialog, close.tex[Button::DOWN]));
	c->AddEntry("closeButtonDisabled", Entry::AreaLayout, offsetof(layout::PickItemDialog, close.tex[Button::DISABLED]));
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::PickItemDialog, background));
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::PickItemDialog, box));
	c->AddEntry("font", Entry::Font, offsetof(layout::PickItemDialog, font));

	c = AddControl<layout::Scrollbar>("Scrollbar");
	c->AddEntry("tex", Entry::AreaLayout, offsetof(layout::Scrollbar, tex));
	c->AddEntry("tex2", Entry::AreaLayout, offsetof(layout::Scrollbar, tex2));

	c = AddControl<layout::Slider>("Slider");
	c->AddEntry("font", Entry::Font, offsetof(layout::Slider, font));

	c = AddControl<layout::SplitPanel>("SplitPanel");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::SplitPanel, background));
	c->AddEntry("horizontal", Entry::AreaLayout, offsetof(layout::SplitPanel, horizontal));
	c->AddEntry("vertical", Entry::AreaLayout, offsetof(layout::SplitPanel, vertical));

	c = AddControl<layout::TabControl>("TabControl");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::TabControl, background));
	c->AddEntry("line", Entry::AreaLayout, offsetof(layout::TabControl, line));
	c->AddEntry("button", Entry::AreaLayout, offsetof(layout::TabControl, button));
	c->AddEntry("buttonHover", Entry::AreaLayout, offsetof(layout::TabControl, buttonHover));
	c->AddEntry("buttonDown", Entry::AreaLayout, offsetof(layout::TabControl, buttonDown));
	c->AddEntry("font", Entry::Font, offsetof(layout::TabControl, font));
	c->AddEntry("padding", Entry::Int2, offsetof(layout::TabControl, padding));
	c->AddEntry("paddingActive", Entry::Int2, offsetof(layout::TabControl, paddingActive));
	c->AddEntry("close", Entry::AreaLayout, offsetof(layout::TabControl, close));
	c->AddEntry("closeHover", Entry::AreaLayout, offsetof(layout::TabControl, closeHover));
	c->AddEntry("buttonPrev", Entry::AreaLayout, offsetof(layout::TabControl, buttonPrev));
	c->AddEntry("buttonPrevHover", Entry::AreaLayout, offsetof(layout::TabControl, buttonPrevHover));
	c->AddEntry("buttonNext", Entry::AreaLayout, offsetof(layout::TabControl, buttonNext));
	c->AddEntry("buttonNextHover", Entry::AreaLayout, offsetof(layout::TabControl, buttonNextHover));

	c = AddControl<layout::TextBox>("TextBox");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::TextBox, background));
	c->AddEntry("font", Entry::Font, offsetof(layout::TextBox, font));

	c = AddControl<layout::TooltipController>("TooltipController");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::TooltipController, box));
	c->AddEntry("font", Entry::Font, offsetof(layout::TooltipController, font));
	c->AddEntry("fontBig", Entry::Font, offsetof(layout::TooltipController, fontBig));
	c->AddEntry("fontSmall", Entry::Font, offsetof(layout::TooltipController, fontSmall));

	c = AddControl<layout::TreeView>("TreeView");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::TreeView, background));
	c->AddEntry("selected", Entry::AreaLayout, offsetof(layout::TreeView, selected));
	c->AddEntry("button", Entry::AreaLayout, offsetof(layout::TreeView, button));
	c->AddEntry("buttonHover", Entry::AreaLayout, offsetof(layout::TreeView, buttonHover));
	c->AddEntry("buttonDown", Entry::AreaLayout, offsetof(layout::TreeView, buttonDown));
	c->AddEntry("buttonDownHover", Entry::AreaLayout, offsetof(layout::TreeView, buttonDownHover));
	c->AddEntry("font", Entry::Font, offsetof(layout::TreeView, font));
	c->AddEntry("fontColor", Entry::Color, offsetof(layout::TreeView, fontColor));
	c->AddEntry("levelOffset", Entry::Int, offsetof(layout::TreeView, levelOffset));
	c->AddEntry("textBoxBackground", Entry::Image, offsetof(layout::TreeView, textBoxBackground));
	c->AddEntry("dragAndDrop", Entry::Image, offsetof(layout::TreeView, dragAndDrop));

	c = AddControl<layout::Window>("Window");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::Window, background));
	c->AddEntry("header", Entry::AreaLayout, offsetof(layout::Window, header));
	c->AddEntry("font", Entry::Font, offsetof(layout::Window, font));
	c->AddEntry("fontColor", Entry::Color, offsetof(layout::Window, fontColor));
	c->AddEntry("headerHeight", Entry::Int, offsetof(layout::Window, headerHeight));
	c->AddEntry("padding", Entry::Int2, offsetof(layout::Window, padding));
}

//=================================================================================================
LayoutLoader::Control* LayoutLoader::AddControl(cstring name, uint size, const type_info& type)
{
	Control* control = new Control(name, size, type);
	controls[name] = control;
	return control;
}
