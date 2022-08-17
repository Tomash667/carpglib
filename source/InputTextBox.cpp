#include "Pch.h"
#include "InputTextBox.h"

#include "Input.h"

//=================================================================================================
InputTextBox::InputTextBox() : added(false), lose_focus(true), esc_clear(true), background_color(Color::None), max_lines(100), max_cache(10)
{
}

//=================================================================================================
void InputTextBox::Draw(ControlDrawData*)
{
	// t�o
	if(background_color != Color::None)
	{
		Rect r0 = { global_pos.x, global_pos.y, global_pos.x + textbox_size.x, global_pos.y + textbox_size.y };
		gui->DrawArea(background_color, r0);

		r0.Top() = inputbox_pos.y;
		r0.Bottom() = r0.Top() + inputbox_size.y;
		gui->DrawArea(background_color, r0);
	}

	// box na tekst
	gui->DrawArea(Box2d::Create(global_pos, textbox_size), layout->box);

	// box na input
	gui->DrawArea(Box2d::Create(inputbox_pos, inputbox_size), layout->box);

	// tekst
	Rect rc = { global_pos.x + 4, global_pos.y + 4, global_pos.x + textbox_size.x - 4, global_pos.y + textbox_size.y - 4 };
	Rect r = { rc.Left(), rc.Top() - int(scrollbar.offset), rc.Right(), rc.Bottom() - int(scrollbar.offset) - 20 };
	gui->DrawText(layout->font, text, 0, Color::Black, r, &rc, nullptr, nullptr, &lines);

	// input
	Rect r2 = { inputbox_pos.x + 4, inputbox_pos.y, inputbox_pos.x + inputbox_size.x - 4, inputbox_pos.y + inputbox_size.y };
	gui->DrawText(layout->font, caret_blink >= 0.f ? Format("%s|", input_str.c_str()) : input_str, DTF_LEFT | DTF_VCENTER, Color::Black, r2, &r2);

	// scrollbar
	scrollbar.Draw();
}

//=================================================================================================
void InputTextBox::Update(float dt)
{
	if(mouse_focus)
	{
		if(input->Focus() && IsInside(gui->cursorPos))
			scrollbar.ApplyMouseWheel();

		bool release_key = false;
		if(PointInRect(gui->cursorPos, inputbox_pos, inputbox_size))
		{
			gui->cursorMode = CURSOR_TEXT;
			if(!focus && input->Focus() && input->PressedRelease(Key::LeftButton))
				focus = true;
		}
		else if(focus && input->Focus() && input->Pressed(Key::LeftButton))
		{
			focus = false;
			release_key = true;
		}

		scrollbar.mouse_focus = mouse_focus;
		scrollbar.Update(dt);
		if(release_key)
			input->Released(Key::LeftButton);
	}
	if(focus)
	{
		caret_blink += dt * 2;
		if(caret_blink >= 1.f)
			caret_blink = -1.f;
		if(input->Focus())
		{
			if(input->PressedRelease(Key::Up))
			{
				// poprzednia komenda
				if(input_counter == -1)
				{
					input_counter = last_input_counter - 1;
					if(input_counter != -1)
						input_str = cache[input_counter];
				}
				else
				{
					--input_counter;
					if(input_counter == -1)
						input_counter = last_input_counter - 1;
					input_str = cache[input_counter];
				}
			}
			else if(input->PressedRelease(Key::Down))
			{
				// nast�pna komenda
				++input_counter;
				if(input_counter == last_input_counter)
				{
					if(last_input_counter == 0)
						input_counter = -1;
					else
						input_counter = 0;
				}
				if(input_counter != -1)
					input_str = cache[input_counter];
			}
			if(input->PressedRelease(Key::Enter))
			{
				if(!input_str.empty())
				{
					// dodaj ostatni� komend�
					if(last_input_counter == 0 || cache[last_input_counter - 1] != input_str)
					{
						if(last_input_counter == max_cache)
						{
							for(int i = 0; i < max_cache - 1; ++i)
								cache[i] = cache[i + 1];
							cache[max_cache - 1] = input_str;
						}
						else
						{
							cache[last_input_counter] = input_str;
							++last_input_counter;
						}
					}
					// wykonaj
					event(input_str);
					// wyczy��
					input_str.clear();
					input_counter = -1;
				}
				if(lose_focus)
				{
					focus = false;
					Event(GuiEvent_LostFocus);
				}
			}
			else if(esc_clear && input->PressedRelease(Key::Escape))
			{
				input_str.clear();
				input_counter = -1;
				if(lose_focus)
				{
					focus = false;
					Event(GuiEvent_LostFocus);
				}
			}
		}
	}
	else
		caret_blink = -1.f;

	if(focus)
	{
		if(!added)
		{
			caret_blink = 0.f;
			added = true;
			gui->AddOnCharHandler(this);
		}
	}
	else
	{
		if(added)
		{
			caret_blink = -1.f;
			added = false;
			gui->RemoveOnCharHandler(this);
		}
	}
}

//=================================================================================================
void InputTextBox::Event(GuiEvent e)
{
	if(e == GuiEvent_LostFocus)
	{
		scrollbar.LostFocus();
		if(added)
		{
			caret_blink = -1.f;
			added = false;
			gui->RemoveOnCharHandler(this);
		}
	}
	else if(e == GuiEvent_Moved)
	{
		global_pos = pos + parent->global_pos;
		inputbox_pos = global_pos + Int2(0, textbox_size.y + 6);
		scrollbar.global_pos = global_pos + scrollbar.pos;
	}
	else if(e == GuiEvent_Resize)
	{
		global_pos = parent->global_pos;
		size = parent->size;
		textbox_size = size - Int2(18, 30);
		inputbox_pos = global_pos + Int2(0, textbox_size.y + 6);
		inputbox_size = Int2(textbox_size.x, 24);
		scrollbar.pos = Int2(textbox_size.x + 2, 0);
		scrollbar.global_pos = global_pos + scrollbar.pos;
		scrollbar.size = Int2(16, textbox_size.y);
		scrollbar.part = textbox_size.y - 8;

		size_t OutBegin, OutEnd, InOutIndex = 0;
		int OutWidth, Width = textbox_size.x - 8;
		cstring Text = text.c_str();
		size_t TextEnd = text.length();

		bool skip_to_end = (int(scrollbar.offset) >= (scrollbar.total - scrollbar.part));

		// podziel tekst na linijki
		lines.clear();
		while(layout->font->SplitLine(OutBegin, OutEnd, OutWidth, InOutIndex, Text, TextEnd, 0, Width))
			lines.push_back(TextLine(OutBegin, OutEnd, OutWidth));

		CheckLines();

		scrollbar.total = lines.size()*layout->font->height;
		if(skip_to_end)
		{
			scrollbar.offset = float(scrollbar.total - scrollbar.part);
			if(scrollbar.offset + scrollbar.part > scrollbar.total)
				scrollbar.offset = float(scrollbar.total - scrollbar.part);
			if(scrollbar.offset < 0)
				scrollbar.offset = 0;
		}
	}
	else if(e == GuiEvent_GainFocus)
	{
		if(!added)
		{
			caret_blink = 0.f;
			added = true;
			gui->AddOnCharHandler(this);
		}
	}
}

//=================================================================================================
void InputTextBox::OnChar(char c)
{
	if(c == 0x08)
	{
		// backspace
		if(!input_str.empty())
			input_str.resize(input_str.size() - 1);
	}
	else if(c == 0x0D)
	{
		// pomi� znak
	}
	else
		input_str.push_back(c);
}

//=================================================================================================
void InputTextBox::Init()
{
	textbox_size = size - Int2(18, 30);
	inputbox_pos = global_pos + Int2(0, textbox_size.y + 6);
	inputbox_size = Int2(textbox_size.x, 24);
	scrollbar.pos = Int2(textbox_size.x + 2, 0);
	scrollbar.size = Int2(16, textbox_size.y);
	scrollbar.total = 0;
	scrollbar.part = textbox_size.y - 8;
	scrollbar.offset = 0.f;
	scrollbar.global_pos = global_pos + scrollbar.pos;
	cache.resize(max_cache);
	Reset();
}

//=================================================================================================
void InputTextBox::Reset(bool reset_cache)
{
	input_str.clear();
	text.clear();
	lines.clear();
	input_counter = -1;
	caret_blink = 0.f;
	scrollbar.offset = 0.f;
	scrollbar.total = 0;
	if(reset_cache)
		last_input_counter = 0;
}

//=================================================================================================
void InputTextBox::Add(Cstring str)
{
	if(!text.empty())
		text += '\n';
	size_t InOutIndex = text.length();
	text += str.s;

	size_t OutBegin, OutEnd;
	int OutWidth, Width = textbox_size.x - 8;
	cstring Text = text.c_str();
	size_t TextEnd = text.length();

	bool skip_to_end = (int(scrollbar.offset) >= (scrollbar.total - scrollbar.part));

	// podziel tekst na linijki
	while(layout->font->SplitLine(OutBegin, OutEnd, OutWidth, InOutIndex, Text, TextEnd, 0, Width))
		lines.push_back(TextLine(OutBegin, OutEnd, OutWidth));

	// usu� nadmiarowe linijki z pocz�tku
	CheckLines();

	scrollbar.total = lines.size()*layout->font->height;
	if(skip_to_end)
	{
		scrollbar.offset = float(scrollbar.total - scrollbar.part);
		if(scrollbar.offset + scrollbar.part > scrollbar.total)
			scrollbar.offset = float(scrollbar.total - scrollbar.part);
		if(scrollbar.offset < 0)
			scrollbar.offset = 0;
	}
}

//=================================================================================================
void InputTextBox::CheckLines()
{
	if((int)lines.size() > max_lines)
	{
		lines.erase(lines.begin(), lines.begin() + lines.size() - max_lines);
		int offset = lines[0].begin;
		text.erase(0, offset);
		for(vector<TextLine>::iterator it = lines.begin(), end = lines.end(); it != end; ++it)
		{
			it->begin -= offset;
			it->end -= offset;
		}
	}
}
