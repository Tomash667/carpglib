#include "Pch.h"
#include "InputTextBox.h"

#include "Input.h"

//=================================================================================================
InputTextBox::InputTextBox() : added(false), lose_focus(false), esc_clear(true), background_color(Color::None), max_lines(100), max_cache(10)
{
}

//=================================================================================================
void InputTextBox::Draw()
{
	// t³o
	if(backgroundColor != Color::None)
	{
		Rect r0 = { globalPos.x, globalPos.y, globalPos.x + textboxSize.x, globalPos.y + textboxSize.y };
		gui->DrawArea(backgroundColor, r0);

		r0.Top() = inputboxPos.y;
		r0.Bottom() = r0.Top() + inputboxSize.y;
		gui->DrawArea(backgroundColor, r0);
	}

	// box na tekst
	gui->DrawArea(Box2d::Create(globalPos, textboxSize), layout->box);

	// box na input
	gui->DrawArea(Box2d::Create(inputboxPos, inputboxSize), layout->box);

	// tekst
	Rect rc = { globalPos.x + 4, globalPos.y + 4, globalPos.x + textboxSize.x - 4, globalPos.y + textboxSize.y - 4 };
	Rect r = { rc.Left(), rc.Top() - int(scrollbar.offset), rc.Right(), rc.Bottom() - int(scrollbar.offset) - 20 };
	gui->DrawText(layout->font, text, 0, Color::Black, r, &rc, nullptr, nullptr, &lines);

	// input
	Rect r2 = { inputboxPos.x + 4, inputboxPos.y, inputboxPos.x + inputboxSize.x - 4, inputboxPos.y + inputboxSize.y };
	gui->DrawText(layout->font, caretBlink >= 0.f ? Format("%s|", inputStr.c_str()) : inputStr, DTF_LEFT | DTF_VCENTER, Color::Black, r2, &r2);

	// scrollbar
	scrollbar.Draw();
}

//=================================================================================================
void InputTextBox::Update(float dt)
{
	if(mouseFocus)
	{
		if(input->Focus() && IsInside(gui->cursorPos))
			scrollbar.ApplyMouseWheel();

		bool releaseKey = false;
		if(Rect::IsInside(gui->cursorPos, inputboxPos, inputboxSize))
		{
			gui->SetCursorMode(CURSOR_TEXT);
			if(!focus && input->Focus() && input->PressedRelease(Key::LeftButton))
				focus = true;
		}
		else if(focus && input->Focus() && input->Pressed(Key::LeftButton))
		{
			focus = false;
			releaseKey = true;
		}

		scrollbar.mouseFocus = mouseFocus;
		scrollbar.Update(dt);
		if(releaseKey)
			input->Released(Key::LeftButton);
	}

	if(focus)
	{
		caretBlink += dt * 2;
		if(caretBlink >= 1.f)
			caretBlink = -1.f;
		if(input->Focus())
		{
			if(input->PressedRelease(Key::Up))
			{
				// poprzednia komenda
				if(inputCounter == -1)
				{
					inputCounter = lastInputCounter - 1;
					if(inputCounter != -1)
						inputStr = cache[inputCounter];
				}
				else
				{
					--inputCounter;
					if(inputCounter == -1)
						inputCounter = lastInputCounter - 1;
					inputStr = cache[inputCounter];
				}
			}
			else if(input->PressedRelease(Key::Down))
			{
				// nastêpna komenda
				++inputCounter;
				if(inputCounter == lastInputCounter)
				{
					if(lastInputCounter == 0)
						inputCounter = -1;
					else
						inputCounter = 0;
				}
				if(inputCounter != -1)
					inputStr = cache[inputCounter];
			}
			if(input->PressedRelease(Key::Enter))
			{
				if(!inputStr.empty())
				{
					// dodaj ostatni¹ komendê
					if(lastInputCounter == 0 || cache[lastInputCounter - 1] != inputStr)
					{
						if(lastInputCounter == maxCache)
						{
							for(int i = 0; i < maxCache - 1; ++i)
								cache[i] = cache[i + 1];
							cache[maxCache - 1] = inputStr;
						}
						else
						{
							cache[lastInputCounter] = inputStr;
							++lastInputCounter;
						}
					}
					// wykonaj
					event(inputStr);
					// wyczyœæ
					inputStr.clear();
					inputCounter = -1;
				}
				if(loseFocus)
				{
					focus = false;
					Event(GuiEvent_LostFocus);
				}
			}
			else if(escClear && input->PressedRelease(Key::Escape))
			{
				inputStr.clear();
				inputCounter = -1;
				if(loseFocus)
				{
					focus = false;
					Event(GuiEvent_LostFocus);
				}
			}
		}
	}
	else
		caretBlink = -1.f;

	if(focus)
	{
		if(!added)
		{
			caretBlink = 0.f;
			added = true;
			gui->AddOnCharHandler(this);
		}
	}
	else
	{
		if(added)
		{
			caretBlink = -1.f;
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
			caretBlink = -1.f;
			added = false;
			gui->RemoveOnCharHandler(this);
		}
	}
	else if(e == GuiEvent_Moved)
	{
		globalPos = pos + parent->globalPos;
		inputboxPos = globalPos + Int2(0, textboxSize.y + 6);
		scrollbar.globalPos = globalPos + scrollbar.pos;
	}
	else if(e == GuiEvent_Resize)
	{
		globalPos = parent->globalPos;
		size = parent->size;
		textboxSize = size - Int2(18, 30);
		inputboxPos = globalPos + Int2(0, textboxSize.y + 6);
		inputboxSize = Int2(textboxSize.x, 24);
		scrollbar.pos = Int2(textboxSize.x + 2, 0);
		scrollbar.globalPos = globalPos + scrollbar.pos;
		scrollbar.size = Int2(16, textboxSize.y);
		scrollbar.part = textboxSize.y - 8;

		size_t outBegin, outEnd, inOutIndex = 0;
		int outWidth, Width = textboxSize.x - 8;
		size_t textEnd = text.length();
		bool skipToEnd = (int(scrollbar.offset) >= (scrollbar.total - scrollbar.part));

		// podziel tekst na linijki
		lines.clear();
		while(layout->font->SplitLine(outBegin, outEnd, outWidth, inOutIndex, text.c_str(), textEnd, 0, Width))
			lines.push_back(TextLine(outBegin, outEnd, outWidth));

		CheckLines();

		scrollbar.total = lines.size()*layout->font->height;
		if(skipToEnd)
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
			caretBlink = 0.f;
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
		if(!inputStr.empty())
			inputStr.resize(inputStr.size() - 1);
	}
	else if(c == 0x0D)
	{
		// pomiñ znak
	}
	else
		inputStr.push_back(c);
}

//=================================================================================================
void InputTextBox::Init()
{
	textboxSize = size - Int2(18, 30);
	inputboxPos = globalPos + Int2(0, textboxSize.y + 6);
	inputboxSize = Int2(textboxSize.x, 24);
	scrollbar.pos = Int2(textboxSize.x + 2, 0);
	scrollbar.size = Int2(16, textboxSize.y);
	scrollbar.total = 0;
	scrollbar.part = textboxSize.y - 8;
	scrollbar.offset = 0.f;
	scrollbar.globalPos = globalPos + scrollbar.pos;
	cache.resize(maxCache);
	Reset();
}

//=================================================================================================
void InputTextBox::Reset(bool resetCache)
{
	inputStr.clear();
	text.clear();
	lines.clear();
	inputCounter = -1;
	caretBlink = 0.f;
	scrollbar.offset = 0.f;
	scrollbar.total = 0;
	if(resetCache)
		lastInputCounter = 0;
}

//=================================================================================================
void InputTextBox::Add(Cstring str)
{
	if(!text.empty())
		text += '\n';
	size_t inOutIndex = text.length();
	text += str.s;

	size_t outBegin, outEnd;
	int outWidth, Width = textboxSize.x - 8;
	size_t textEnd = text.length();
	bool skipToEnd = (int(scrollbar.offset) >= (scrollbar.total - scrollbar.part));

	// podziel tekst na linijki
	while(layout->font->SplitLine(outBegin, outEnd, outWidth, inOutIndex, text.c_str(), textEnd, 0, Width))
		lines.push_back(TextLine(outBegin, outEnd, outWidth));

	// usuñ nadmiarowe linijki z pocz¹tku
	CheckLines();

	scrollbar.total = lines.size()*layout->font->height;
	if(skipToEnd)
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
	if((int)lines.size() > maxLines)
	{
		lines.erase(lines.begin(), lines.begin() + lines.size() - maxLines);
		int offset = lines[0].begin;
		text.erase(0, offset);
		for(vector<TextLine>::iterator it = lines.begin(), end = lines.end(); it != end; ++it)
		{
			it->begin -= offset;
			it->end -= offset;
		}
	}
}
