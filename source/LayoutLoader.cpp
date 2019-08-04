#include "EnginePch.h"
#include "EngineCore.h"
#include "LayoutLoader.h"
#include "ResourceManager.h"
// controls
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
	FK_TEX_SIZE,
	FK_OUTLINE
};

//=================================================================================================
LayoutLoader::~LayoutLoader()
{
	DeleteElements(controls);
}

//=================================================================================================
Layout* LayoutLoader::LoadFromFile(cstring path)
{
	if(!initialized)
	{
		t.SetFlags(Tokenizer::F_JOIN_MINUS | Tokenizer::F_MULTI_KEYWORDS | Tokenizer::F_UNESCAPE);
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
		Error("Failed to load layout '%s': %s", path, ex.ToString());
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

	string n = name, font_name;
	int size = -1, weight = 5, outline = 0, tex_size = 0;

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
			font_name = t.MustGetString();
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
		case FK_TEX_SIZE:
			tex_size = t.MustGetInt();
			if(tex_size < 128 || !IsPow2(tex_size))
				t.Throw("Invalid texture size '%d'.", tex_size);
			break;
		}
		t.Next();
	}

	if(font_name.empty() || size < 1)
		t.Throw("Font name or size not set.");

	Font* font = gui->CreateFont(font_name.c_str(), size, weight * 100, tex_size, outline);
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
		const string& entry_name = t.MustGetItemKeyword();
		Entry* entry = c->FindEntry(entry_name);
		if(!entry)
			t.Throw("Missing control '%s' entry '%s'.", c->name.c_str(), entry_name.c_str());
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
					area.border_color = Color::Black;
					area.width = 1;
					break;
				case AreaLayout::Mode::Image:
					area.tex = nullptr;
					area.color = Color::White;
					area.background_color = Color::None;
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
							const string& img_name = t.MustGetString();
							area.tex = ResourceManager::Get().Load<Texture>(img_name);
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
							t.Throw("This area layout don't support 'border_color' entry.");
						t.Parse(area.border_color);
						break;
					case AK_BACKGROUND_COLOR:
						if(area.mode != AreaLayout::Mode::Image)
							t.Throw("This area layout don't support 'background_color' entry.");
						t.Parse(area.background_color);
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
				const string& font_name = t.MustGetString();
				Font* font = FindFont(font_name);
				if(!font)
					t.Throw("Missing font '%s'.", font_name.c_str());
				entry->GetValue<Font*>(data) = font;
				t.Next();
			}
			break;
		case Entry::Image:
			{
				const string& img_name = t.MustGetString();
				entry->GetValue<Texture*>(data) = ResourceManager::Get().Load<Texture>(img_name);
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
		}
	}
}

//=================================================================================================
void LayoutLoader::RegisterKeywords()
{
	t.AddKeywords(G_TOP, {
		{"register_font",TK_REGISTER_FONT},
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
		{"border_color",AK_BORDER_COLOR},
		{"background_color",AK_BACKGROUND_COLOR}
		});

	t.AddKeywords(G_FONT, {
		{"name",FK_NAME},
		{"size",FK_SIZE},
		{"weight",FK_WEIGHT},
		{"tex_size",FK_TEX_SIZE},
		{"outline",FK_OUTLINE}
		});

	t.AddEnums<AreaLayout::Mode>(G_AREA_MODE, {
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
	c->AddEntry("tex_hover", Entry::AreaLayout, offsetof(layout::Button, tex[Button::HOVER]));
	c->AddEntry("tex_down", Entry::AreaLayout, offsetof(layout::Button, tex[Button::DOWN]));
	c->AddEntry("tex_disabled", Entry::AreaLayout, offsetof(layout::Button, tex[Button::DISABLED]));
	c->AddEntry("font", Entry::Font, offsetof(layout::Button, font));

	c = AddControl<layout::CheckBox>("CheckBox");
	c->AddEntry("tex", Entry::AreaLayout, offsetof(layout::CheckBox, tex[CheckBox::NONE]));
	c->AddEntry("tex_hover", Entry::AreaLayout, offsetof(layout::CheckBox, tex[CheckBox::HOVER]));
	c->AddEntry("tex_down", Entry::AreaLayout, offsetof(layout::CheckBox, tex[CheckBox::DOWN]));
	c->AddEntry("tex_disabled", Entry::AreaLayout, offsetof(layout::CheckBox, tex[CheckBox::DISABLED]));
	c->AddEntry("font", Entry::Font, offsetof(layout::CheckBox, font));
	c->AddEntry("tick", Entry::AreaLayout, offsetof(layout::CheckBox, tick));

	c = AddControl<layout::CheckBoxGroup>("CheckBoxGroup");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::CheckBoxGroup, background));
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::CheckBoxGroup, box));
	c->AddEntry("checked", Entry::AreaLayout, offsetof(layout::CheckBoxGroup, checked));
	c->AddEntry("font", Entry::Font, offsetof(layout::CheckBoxGroup, font));
	c->AddEntry("font_color", Entry::Color, offsetof(layout::CheckBoxGroup, font_color));

	c = AddControl<layout::DialogBox>("DialogBox");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::DialogBox, background));
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::DialogBox, box));
	c->AddEntry("font", Entry::Font, offsetof(layout::DialogBox, font));

	c = AddControl<layout::FlowContainer>("FlowContainer");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::FlowContainer, box));
	c->AddEntry("selection", Entry::AreaLayout, offsetof(layout::FlowContainer, selection));
	c->AddEntry("font", Entry::Font, offsetof(layout::FlowContainer, font));
	c->AddEntry("font_section", Entry::Font, offsetof(layout::FlowContainer, font_section));

	c = AddControl<layout::Grid>("Grid");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::Grid, box));
	c->AddEntry("font", Entry::Font, offsetof(layout::Grid, font));

	c = AddControl<layout::Gui>("Gui");
	c->AddEntry("cursor", Entry::Image, offsetof(layout::Gui, cursor[CURSOR_NORMAL]));
	c->AddEntry("cursor_hover", Entry::Image, offsetof(layout::Gui, cursor[CURSOR_HOVER]));
	c->AddEntry("cursor_text", Entry::Image, offsetof(layout::Gui, cursor[CURSOR_TEXT]));

	c = AddControl<layout::InputTextBox>("InputTextBox");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::InputTextBox, box));
	c->AddEntry("font", Entry::Font, offsetof(layout::InputTextBox, font));

	c = AddControl<layout::Label>("Label");
	c->AddEntry("font", Entry::Font, offsetof(layout::Label, font));
	c->AddEntry("color", Entry::Color, offsetof(layout::Label, color));

	c = AddControl<layout::ListBox>("ListBox");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::ListBox, box));
	c->AddEntry("selection", Entry::AreaLayout, offsetof(layout::ListBox, selection));
	c->AddEntry("down_arrow", Entry::Image, offsetof(layout::ListBox, down_arrow));
	c->AddEntry("font", Entry::Font, offsetof(layout::ListBox, font));

	c = AddControl<layout::MenuBar>("MenuBar");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::MenuBar, background));
	c->AddEntry("button", Entry::AreaLayout, offsetof(layout::MenuBar, button));
	c->AddEntry("button_hover", Entry::AreaLayout, offsetof(layout::MenuBar, button_hover));
	c->AddEntry("button_down", Entry::AreaLayout, offsetof(layout::MenuBar, button_down));
	c->AddEntry("font", Entry::Font, offsetof(layout::MenuBar, font));
	c->AddEntry("padding", Entry::Int2, offsetof(layout::MenuBar, padding));
	c->AddEntry("item_padding", Entry::Int2, offsetof(layout::MenuBar, item_padding));
	c->AddEntry("font_color", Entry::Color, offsetof(layout::MenuBar, font_color));
	c->AddEntry("font_color_hover", Entry::Color, offsetof(layout::MenuBar, font_color_hover));
	c->AddEntry("font_color_down", Entry::Color, offsetof(layout::MenuBar, font_color_down));

	c = AddControl<layout::MenuList>("MenuList");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::MenuList, box));
	c->AddEntry("selection", Entry::AreaLayout, offsetof(layout::MenuList, selection));
	c->AddEntry("font", Entry::Font, offsetof(layout::MenuList, font));

	c = AddControl<layout::MenuStrip>("MenuStrip");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::MenuStrip, background));
	c->AddEntry("button_hover", Entry::AreaLayout, offsetof(layout::MenuStrip, button_hover));
	c->AddEntry("font", Entry::Font, offsetof(layout::MenuStrip, font));
	c->AddEntry("padding", Entry::Int2, offsetof(layout::MenuStrip, padding));
	c->AddEntry("item_padding", Entry::Int2, offsetof(layout::MenuStrip, item_padding));
	c->AddEntry("font_color_disabled", Entry::Color, offsetof(layout::MenuStrip, font_color_disabled));

	c = AddControl<layout::Notifications>("Notifications");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::Notifications, box));
	c->AddEntry("font", Entry::Font, offsetof(layout::Notifications, font));

	c = AddControl<layout::Overlay>("Overlay");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::Overlay, background));

	c = AddControl<layout::Panel>("Panel");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::Panel, background));

	c = AddControl<layout::PickFileDialog>("PickFileDialog");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::PickFileDialog, box));

	c = AddControl<layout::PickItemDialog>("PickItemDialog");
	c->AddEntry("close_button", Entry::AreaLayout, offsetof(layout::PickItemDialog, close.tex[Button::NONE]));
	c->AddEntry("close_button_hover", Entry::AreaLayout, offsetof(layout::PickItemDialog, close.tex[Button::HOVER]));
	c->AddEntry("close_button_down", Entry::AreaLayout, offsetof(layout::PickItemDialog, close.tex[Button::DOWN]));
	c->AddEntry("close_button_disabled", Entry::AreaLayout, offsetof(layout::PickItemDialog, close.tex[Button::DISABLED]));
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
	c->AddEntry("button_hover", Entry::AreaLayout, offsetof(layout::TabControl, button_hover));
	c->AddEntry("button_down", Entry::AreaLayout, offsetof(layout::TabControl, button_down));
	c->AddEntry("font", Entry::Font, offsetof(layout::TabControl, font));
	c->AddEntry("padding", Entry::Int2, offsetof(layout::TabControl, padding));
	c->AddEntry("padding_active", Entry::Int2, offsetof(layout::TabControl, padding_active));
	c->AddEntry("close", Entry::AreaLayout, offsetof(layout::TabControl, close));
	c->AddEntry("close_hover", Entry::AreaLayout, offsetof(layout::TabControl, close_hover));
	c->AddEntry("button_prev", Entry::AreaLayout, offsetof(layout::TabControl, button_prev));
	c->AddEntry("button_prev_hover", Entry::AreaLayout, offsetof(layout::TabControl, button_prev_hover));
	c->AddEntry("button_next", Entry::AreaLayout, offsetof(layout::TabControl, button_next));
	c->AddEntry("button_next_hover", Entry::AreaLayout, offsetof(layout::TabControl, button_next_hover));

	c = AddControl<layout::TextBox>("TextBox");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::TextBox, background));
	c->AddEntry("font", Entry::Font, offsetof(layout::TextBox, font));

	c = AddControl<layout::TooltipController>("TooltipController");
	c->AddEntry("box", Entry::AreaLayout, offsetof(layout::TooltipController, box));
	c->AddEntry("font", Entry::Font, offsetof(layout::TooltipController, font));
	c->AddEntry("font_big", Entry::Font, offsetof(layout::TooltipController, font_big));
	c->AddEntry("font_small", Entry::Font, offsetof(layout::TooltipController, font_small));

	c = AddControl<layout::TreeView>("TreeView");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::TreeView, background));
	c->AddEntry("button", Entry::AreaLayout, offsetof(layout::TreeView, button));
	c->AddEntry("button_hover", Entry::AreaLayout, offsetof(layout::TreeView, button_hover));
	c->AddEntry("button_down", Entry::AreaLayout, offsetof(layout::TreeView, button_down));
	c->AddEntry("button_down_hover", Entry::AreaLayout, offsetof(layout::TreeView, button_down_hover));
	c->AddEntry("selected", Entry::AreaLayout, offsetof(layout::TreeView, selected));
	c->AddEntry("text_box_background", Entry::Image, offsetof(layout::TreeView, text_box_background));
	c->AddEntry("drag_n_drop", Entry::Image, offsetof(layout::TreeView, drag_n_drop));
	c->AddEntry("font", Entry::Font, offsetof(layout::TreeView, font));
	c->AddEntry("level_offset", Entry::Int, offsetof(layout::TreeView, level_offset));

	c = AddControl<layout::Window>("Window");
	c->AddEntry("background", Entry::AreaLayout, offsetof(layout::Window, background));
	c->AddEntry("header", Entry::AreaLayout, offsetof(layout::Window, header));
	c->AddEntry("font", Entry::Font, offsetof(layout::Window, font));
	c->AddEntry("header_height", Entry::Int, offsetof(layout::Window, header_height));
	c->AddEntry("padding", Entry::Int2, offsetof(layout::Window, padding));
}

//=================================================================================================
LayoutLoader::Control* LayoutLoader::AddControl(cstring name, uint size, const type_info& type)
{
	Control* control = new Control(name, size, type);
	controls[name] = control;
	return control;
}
