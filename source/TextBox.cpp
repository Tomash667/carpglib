#include "Pch.h"
#include "TextBox.h"

#include "Input.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
static const int padding = 4;
static const Int2 NOT_SELECTED = Int2(-1, -1);

//=================================================================================================
TextBox::TextBox(bool isNew) : Control(isNew), added(false), multiline(false), numeric(false), label(nullptr), scrollbar(nullptr), readonly(false), caretIndex(-1),
selectStartIndex(-1), down(false), offset(0), offsetMove(0.f), requireScrollbar(false), lastYMove(-1), errorMode(false)
{
}

//=================================================================================================
TextBox::~TextBox()
{
	delete scrollbar;
}

//=================================================================================================
void TextBox::Draw()
{
	if(!isNew)
	{
		cstring txt = (caretBlink >= 0.f ? Format("%s|", text.c_str()) : text.c_str());

		gui->DrawArea(Box2d::Create(globalPos, size), layout->background);

		Rect r = { globalPos.x + padding, globalPos.y + padding, globalPos.x + size.x - padding, globalPos.y + size.y - padding };

		if(!scrollbar)
			gui->DrawText(layout->font, txt, multiline ? DTF_TOP : DTF_VCENTER, layout->font_color, r);
		else
		{
			Rect r2 = Rect(r) - Int2(0, int(scrollbar->offset));
			gui->DrawText(layout->font, txt, DTF_TOP, layout->font_color, r2, &r);
			scrollbar->Draw();
		}

		if(label)
		{
			Rect r = { globalPos.x, globalPos.y - 20, globalPos.x + size.x, globalPos.y };
			gui->DrawText(layout->font, label, 0, layout->font_color, r);
		}
	}
	else
	{
		// not coded yet
		assert(!label);

		Box2d* clipRect = gui->GetClipRect();
		int offsety = (scrollbar ? (int)scrollbar->offset : 0);
		const int lineHeight = layout->font->height;

		// background
		gui->DrawArea(Box2d::Create(globalPos, size), layout->background, clipRect);

		Rect rclip;
		Rect textboxRect = { globalPos.x + padding, globalPos.y + padding, globalPos.x + realSize.x - padding, globalPos.y + realSize.y - padding };
		if(clipRect)
			rclip = Rect::Intersect(Rect(*clipRect), textboxRect);
		else
			rclip = textboxRect;

		// selected
		if(selectStartIndex != NOT_SELECTED && selectStartIndex != selectEndIndex)
		{
			Color color = ((readonly || disabled) ? layout->selection_color_disabled : layout->selection_color);
			int selectStartLine = selectStartPos.y / lineHeight;
			int selectEndLine = selectEndPos.y / lineHeight;
			int lines = selectEndLine - selectStartLine + 1;
			Rect area, r;
			Int2 pos = globalPos - Int2(offset, offsety) + Int2(padding, padding);

			// ...A----B
			// C-------D
			// E----F...
			if(lines == 1)
			{
				// ...A----B... partial line
				r = {
					pos.x + selectStartPos.x,
					pos.y + selectStartPos.y,
					pos.x + selectEndPos.x,
					pos.y + selectStartPos.y + lineHeight
				};
				area = Rect::Intersect(r, rclip);
				gui->DrawArea(color, area.LeftTop(), area.Size());
			}
			else
			{
				// A-B partial top line
				r = {
					pos.x + selectStartPos.x,
					pos.y + selectStartPos.y,
					pos.x + realSize.x,
					pos.y + selectStartPos.y + lineHeight
				};
				area = Rect::Intersect(r, rclip);
				gui->DrawArea(color, area.LeftTop(), area.Size());

				// C-D full middle line(s)
				if(lines > 2)
				{
					r = {
						pos.x,
						pos.y + selectStartPos.y + lineHeight,
						pos.x + realSize.x,
						pos.y + selectEndPos.y
					};
					area = Rect::Intersect(r, rclip);
					gui->DrawArea(color, area.LeftTop(), area.Size());
				}

				// E-F partial bottom line
				r = {
					pos.x,
					pos.y + selectEndPos.y,
					pos.x + selectEndPos.x,
					pos.y + selectEndPos.y + lineHeight
				};
				area = Rect::Intersect(r, rclip);
				gui->DrawArea(color, area.LeftTop(), area.Size());
			}
		}

		// text
		Rect r =
		{
			globalPos.x + padding - offset,
			globalPos.y + padding - offsety,
			globalPos.x + realSize.x - padding,
			globalPos.y + realSize.y - padding
		};
		Rect area = Rect::Intersect(r, rclip);
		int drawFlags = (multiline ? DTF_LEFT : DTF_VCENTER | DTF_SINGLELINE);
		gui->DrawText(layout->font, text, drawFlags, layout->fontColor, r, &area);

		// carret
		if(caretBlink >= 0.f)
		{
			Int2 p(globalPos.x + padding + caretPos.x - offset, globalPos.y + padding + caretPos.y - offsety);
			Rect caretRect = {
				p.x,
				p.y,
				p.x + 1,
				p.y + lineHeight
			};
			Rect caretRectClip;
			if(Rect::Intersect(caretRect, rclip, caretRectClip))
				gui->DrawArea(Color::Black, caretRectClip.LeftTop(), caretRectClip.Size());
		}

		if(requireScrollbar)
			scrollbar->Draw();
	}
}

//=================================================================================================
void TextBox::Update(float dt)
{
	bool clicked = false;

	if(mouseFocus)
	{
		if(Rect::IsInside(gui->cursorPos, globalPos, isNew ? realSize : size))
		{
			gui->SetCursorMode(CURSOR_TEXT);
			if(isNew && (input->PressedRelease(Key::LeftButton) || input->PressedRelease(Key::RightButton)))
			{
				// set caret position, update selection
				bool prevFocus = focus;
				Int2 newIndex, newPos, prevIndex = caretIndex;
				uint charIndex;
				GetCaretPos(gui->cursorPos, newIndex, newPos, &charIndex);
				caretBlink = 0.f;
				TakeFocus(true);
				if(input->Down(Key::Shift) && prevFocus)
					CalculateSelection(newIndex, newPos);
				else
					selectStartIndex = NOT_SELECTED;
				caretIndex = newIndex;
				caretPos = newPos;
				down = true;
				clicked = true;
				lastYMove = -1;

				// double click select whole word
				if(gui->DoubleClick(Key::LeftButton) && !input->Down(Key::Shift) && prevIndex == caretIndex)
				{
					cstring whitespace = " \t\n\r";
					char c = text[charIndex];
					if(strchr(whitespace, c) == nullptr)
					{
						uint pos = charIndex;
						while(pos > 0)
						{
							c = text[pos - 1];
							if(strchr(whitespace, c) != nullptr)
								break;
							--pos;
						}
						uint startPos = pos;
						pos = charIndex;
						while(pos + 1 < text.length())
						{
							c = text[pos + 1];
							if(strchr(whitespace, c) != nullptr)
								break;
							++pos;
						}

						selectStartIndex = layout->font->FromRawIndex(fontLines, startPos);
						selectFixedIndex = selectStartIndex;
						selectStartPos = IndexToPos(selectStartIndex);
						selectEndIndex = layout->font->FromRawIndex(fontLines, pos + 1);
						caretIndex = selectEndIndex;
						selectEndPos = IndexToPos(selectEndIndex);
						caretPos = selectEndPos;
						down = false;
					}
				}
			}
		}
		if(scrollbar && !isNew)
		{
			if(mouseFocus && Rect::IsInside(gui->cursorPos, globalPos, size + Int2(18, 0)))
				scrollbar->ApplyMouseWheel();
			scrollbar->mouseFocus = mouseFocus;
			scrollbar->Update(dt);
		}
	}

	if(scrollbar && isNew)
	{
		if(mouseFocus)
			scrollbar->ApplyMouseWheel();
		UpdateControl(scrollbar, dt);
	}

	if(focus)
	{
		// update caret blinking
		caretBlink += dt * 2;
		if(caretBlink >= 1.f)
			caretBlink = -1.f;

		if(isNew)
		{
			// update selecting with mouse
			if(down && !clicked)
			{
				if(input->Up(Key::LeftButton))
					down = false;
				else
				{
					const int MOVE_SPEED = 300;

					if(!multiline)
					{
						int localX = gui->cursorPos.x - globalPos.x - padding;
						if(localX <= 0.1f * size.x && offset != 0)
						{
							offsetMove -= dt * MOVE_SPEED;
							int offsetMoveI = -(int)ceil(offsetMove);
							offsetMove += offsetMoveI;
							offset -= offsetMoveI;
							if(offset < 0)
								offset = 0;
						}
						else if(localX >= 0.9f * size.x)
						{
							offsetMove += dt * MOVE_SPEED;
							int offsetMoveI = (int)floor(offsetMove);
							offsetMove -= offsetMoveI;
							offset += offsetMoveI;
							const int realSize = size.x - padding * 2;
							const int totalWidth = layout->font->CalculateSize(text).x;
							int maxOffset = totalWidth - realSize;
							if(offset > maxOffset)
								offset = maxOffset;
							if(offset < 0)
								offset = 0;
						}
					}
					else
					{
						int localY = gui->cursorPos.y - globalPos.y - padding;
						float move = 0.f;
						if(localY <= 0.1f * size.y)
							move = -1.f;
						else if(localY >= 0.9f * size.y)
							move = 1.f;
						if(move != 0.f)
						{
							float change = move * MOVE_SPEED * dt;
							scrollbar->UpdateOffset(change);
						}
					}

					Int2 newIndex, newPos;
					GetCaretPos(gui->cursorPos, newIndex, newPos);
					if(newIndex != caretIndex)
					{
						CalculateSelection(newIndex, newPos);
						caretIndex = newIndex;
						caretPos = newPos;
						caretBlink = 0.f;
						lastYMove = -1;
					}
				}
			}

			if(input->DownRepeat(Key::Delete))
			{
				if(selectStartIndex != NOT_SELECTED)
				{
					DeleteSelection();
					CalculateOffset(true);
				}
				else if(!multiline)
				{
					if(caretIndex.x != text.size())
					{
						text.erase(caretIndex.x, 1);
						caretBlink = 0.f;
						UpdateFontLines();
						CalculateOffset(true);
					}
				}
				else
				{
					if(caretIndex.y + 1 != fontLines.size() || caretIndex.x != fontLines[caretIndex.y].count)
					{
						uint index = ToRawIndex(caretIndex);
						text.erase(index, 1);
						caretBlink = 0.f;
						UpdateFontLines();
						caretIndex = layout->font->FromRawIndex(fontLines, index);
						caretPos = IndexToPos(caretIndex);
						CalculateOffset(true);
					}
				}
			}

			// move caret
			bool shift = input->Down(Key::Shift);
			int move = 0;
			if(input->DownRepeat(Key::Up))
				move -= 10;
			if(input->DownRepeat(Key::Down))
				move += 10;
			if(move == 0)
			{
				if(input->DownRepeat(Key::Left))
					move -= 1;
				if(input->DownRepeat(Key::Right))
					move += 1;
			}

			if(move < 0)
			{
				if(!(caretIndex.x > 0 || caretIndex.y > 0))
				{
					move = 0;
					if(!shift && selectStartIndex != NOT_SELECTED)
					{
						selectStartIndex = NOT_SELECTED;
						CalculateOffset(false);
					}
				}
			}
			else if(move > 0)
			{
				if(!(caretIndex.y + 1 != fontLines.size() || caretIndex.x != fontLines[caretIndex.y].count))
				{
					move = 0;
					if(!shift && selectStartIndex != NOT_SELECTED)
					{
						selectStartIndex = NOT_SELECTED;
						CalculateOffset(false);
					}
				}
			}

			if(move != 0)
			{
				Int2 newIndex, newPos;
				if(shift || selectStartIndex == NOT_SELECTED || move == 10 || move == -10)
				{
					if(!shift)
						selectStartIndex = NOT_SELECTED;

					switch(move)
					{
					case -10:
						if(caretIndex.y > 0)
						{
							if(lastYMove == -1)
								lastYMove = caretPos.x;
							Int2 checkPos = globalPos + Int2(lastYMove, caretPos.y - layout->font->height / 2 - (int)scrollbar->offset);
							GetCaretPos(checkPos, newIndex, newPos);
						}
						else
						{
							newIndex = Int2(0, 0);
							newPos = Int2(0, 0);
							lastYMove = -1;
						}
						break;
					case -1:
						if(caretIndex.x > 0)
						{
							newIndex = Int2(caretIndex.x - 1, caretIndex.y);
							uint rawIndex = ToRawIndex(newIndex);
							newPos = caretPos;
							newPos.x -= layout->font->GetCharWidth(text[rawIndex]);
						}
						else
						{
							assert(caretIndex.y > 0);
							newIndex = Int2(fontLines[caretIndex.y - 1].count, caretIndex.y - 1);
							newPos = Int2(fontLines[newIndex.y].width, newIndex.y * layout->font->height);
						}
						lastYMove = -1;
						break;
					case +1:
						if((uint)caretIndex.x < fontLines[caretIndex.y].count)
						{
							newIndex = Int2(caretIndex.x + 1, caretIndex.y);
							uint rawIndex = ToRawIndex(newIndex);
							newPos = caretPos;
							newPos.x += layout->font->GetCharWidth(text[rawIndex - 1]);
						}
						else
						{
							assert((uint)caretIndex.y < fontLines.size());
							newIndex = Int2(0, caretIndex.y + 1);
							newPos = Int2(0, newIndex.y * layout->font->height);
						}
						lastYMove = -1;
						break;
					case +10:
						if(caretIndex.y + 1 < (int)fontLines.size())
						{
							if(lastYMove == -1)
								lastYMove = caretPos.x;
							Int2 checkPos = globalPos + Int2(lastYMove, caretPos.y + layout->font->height * 3 / 2 - (int)scrollbar->offset);
							GetCaretPos(checkPos, newIndex, newPos);
						}
						else
						{
							newIndex = Int2(fontLines.back().count, fontLines.size() - 1);
							newPos = Int2(fontLines.back().width, newIndex.y * layout->font->height);
							lastYMove = -1;
						}
						break;
					}

					if(shift)
						CalculateSelection(newIndex, newPos);
				}
				else if(selectStartIndex != NOT_SELECTED)
				{
					if(move == -1)
					{
						newIndex = selectStartIndex;
						newPos = selectStartPos;
					}
					else
					{
						newIndex = selectEndIndex;
						newPos = selectEndPos;
					}
					selectStartIndex = NOT_SELECTED;
					lastYMove = -1;
				}

				caretIndex = newIndex;
				caretPos = newPos;
				caretBlink = 0.f;
				CalculateOffset(false);
			}

			// select all
			if(input->Shortcut(KEY_CONTROL, Key::A))
			{
				caretIndex = Int2(0, 0);
				caretPos = Int2(0, 0);
				caretBlink = 0.f;
				selectStartIndex = Int2(0, 0);
				selectStartPos = Int2(0, 0);
				selectEndIndex = Int2(fontLines.back().count, fontLines.size() - 1);
				selectEndPos = Int2(fontLines.back().width, fontLines.size() * layout->font->height);
				selectFixedIndex = Int2(0, 0);
			}

			// copy
			if(selectStartIndex != NOT_SELECTED && input->Shortcut(KEY_CONTROL, Key::C))
			{
				uint start = ToRawIndex(selectStartIndex);
				uint end = ToRawIndex(selectEndIndex);
				gui->SetClipboard(text.substr(start, end - start).c_str());
			}

			// paste
			if(!readonly && !disabled && input->Shortcut(KEY_CONTROL, Key::V))
			{
				cstring clipboard = gui->GetClipboard();
				if(clipboard)
				{
					string str = clipboard;
					RemoveEndOfLine(str, !multiline);
					if(selectStartIndex != NOT_SELECTED)
						DeleteSelection();
					uint index = ToRawIndex(caretIndex);
					if(limit <= 0 || text.length() + str.length() <= (uint)limit)
						text.insert(index, str);
					else
					{
						int maxChars = limit - text.length();
						text.insert(index, str, maxChars);
					}
					UpdateFontLines();
					index += str.length();
					caretIndex = layout->font->FromRawIndex(fontLines, index);
					caretPos = IndexToPos(caretIndex);
					CalculateOffset(true);
				}
			}

			// cut
			if(!readonly && selectStartIndex != NOT_SELECTED && input->Shortcut(KEY_CONTROL, Key::X))
			{
				uint start = ToRawIndex(selectStartIndex);
				uint end = ToRawIndex(selectEndIndex);
				gui->SetClipboard(text.substr(start, end - start).c_str());
				DeleteSelection();
				CalculateOffset(true);
			}
		}
	}
	else
	{
		caretBlink = -1.f;
		down = false;
	}
}

//=================================================================================================
void TextBox::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Moved:
		if(scrollbar && isNew)
			scrollbar->globalPos = scrollbar->pos + globalPos;
		break;
	case GuiEvent_Resize:
		if(scrollbar && isNew)
		{
			scrollbar->pos = Int2(size.x - 16, 0);
			scrollbar->size = Int2(16, size.y);
			scrollbar->part = size.y - 8;
			UpdateFontLines();
		}
		break;
	case GuiEvent_GainFocus:
		if(!added)
		{
			if(!isNew)
				caretBlink = 0.f;
			if(!readonly && !disabled)
				gui->AddOnCharHandler(this);
			added = true;
		}
		break;
	case GuiEvent_LostFocus:
		if(added)
		{
			selectStartIndex = NOT_SELECTED;
			caretBlink = -1.f;
			if(!readonly && !disabled)
				gui->RemoveOnCharHandler(this);
			added = false;
			down = false;
			offsetMove = 0.f;
		}
		break;
	case GuiEvent_Initialize:
		if(isNew)
		{
			if(multiline)
			{
				scrollbar = new Scrollbar;
				scrollbar->pos = Int2(size.x - 16, 0);
				scrollbar->size = Int2(16, size.y);
				scrollbar->total = 0;
				scrollbar->part = size.y - 8;
				scrollbar->offset = 0.f;
			}
			UpdateFontLines();
		}
		break;
	}
}

//=================================================================================================
void TextBox::OnChar(char c)
{
	if(c == (char)Key::Backspace)
	{
		// backspace
		if(!isNew)
		{
			if(!text.empty())
			{
				text.pop_back();
				OnTextChanged();
			}
		}
		else
		{
			if(selectStartIndex != NOT_SELECTED)
			{
				DeleteSelection();
				CalculateOffset(true);
			}
			else if(!multiline)
			{
				if(caretIndex.x > 0)
				{
					--caretIndex.x;
					caretPos.x -= layout->font->GetCharWidth(text[caretIndex.x]);
					caretBlink = 0.f;
					text.erase(caretIndex.x, 1);
					UpdateFontLines();
					CalculateOffset(true);
					OnTextChanged();
				}
			}
			else
			{
				if(caretIndex.x > 0 || caretIndex.y > 0)
				{
					uint index = ToRawIndex(caretIndex);
					--index;
					text.erase(index, 1);
					caretBlink = 0.f;
					UpdateFontLines();
					caretIndex = layout->font->FromRawIndex(fontLines, index);
					caretPos = IndexToPos(caretIndex);
					CalculateOffset(true);
					OnTextChanged();
				}
			}
		}
	}
	else if(c == (char)Key::Enter && !multiline)
	{
		// enter - skip
	}
	else
	{
		if(c == '\r')
			c = '\n';

		if(numeric)
		{
			assert(!isNew);
			if(c == '-')
			{
				if(text.empty())
					text.push_back('-');
				else if(text[0] == '-')
					text.erase(text.begin());
				else
					text.insert(text.begin(), '-');
				ValidateNumber();
			}
			else if(c == '+')
			{
				if(!text.empty() && text[0] == '-')
				{
					text.erase(text.begin());
					ValidateNumber();
				}
			}
			else if(isdigit(byte(c)))
			{
				text.push_back(c);
				ValidateNumber();
			}
		}
		else
		{
			if(selectStartIndex != NOT_SELECTED)
				DeleteSelection();
			if(limit <= 0 || limit > (int)text.size())
			{
				if(!isNew)
					text.push_back(c);
				else
				{
					if(!multiline)
					{
						text.insert(caretIndex.x, 1, c);
						caretPos.x += layout->font->GetCharWidth(c);
						++caretIndex.x;
						UpdateFontLines();
					}
					else
					{
						uint index = ToRawIndex(caretIndex);
						text.insert(index, 1, c);
						++index;
						UpdateFontLines();
						caretIndex = layout->font->FromRawIndex(fontLines, index);
						caretPos = IndexToPos(caretIndex);
					}
					caretBlink = 0.f;
					CalculateOffset(true);
				}
				OnTextChanged();
			}
		}
	}
}

//=================================================================================================
void TextBox::ValidateNumber()
{
	int n = atoi(text.c_str());
	if(n < numMin)
		text = Format("%d", numMin);
	else if(n > numMax)
		text = Format("%d", numMax);
}

//=================================================================================================
void TextBox::AddScrollbar()
{
	assert(!isNew); // use SetMultiline
	if(scrollbar)
		return;
	scrollbar = new Scrollbar;
	scrollbar->pos = Int2(size.x + 2, 0);
	scrollbar->size = Int2(16, size.y);
	scrollbar->total = 8;
	scrollbar->part = size.y - 8;
	scrollbar->offset = 0.f;
}

//=================================================================================================
void TextBox::Move(const Int2& movePos)
{
	globalPos = movePos + pos;
	if(scrollbar)
		scrollbar->globalPos = globalPos + scrollbar->pos;
}

//=================================================================================================
void TextBox::Add(cstring str)
{
	assert(!isNew);
	assert(scrollbar);
	Int2 strSize = layout->font->CalculateSize(str, size.x - 8);
	bool skipToEnd = (int(scrollbar->offset) >= (scrollbar->total - scrollbar->part));
	scrollbar->total += strSize.y;
	if(text.empty())
		text = str;
	else
	{
		text += '\n';
		text += str;
	}

	if(skipToEnd)
	{
		scrollbar->offset = float(scrollbar->total - scrollbar->part);
		if(scrollbar->offset < 0.f)
			scrollbar->offset = 0.f;
	}
}

//=================================================================================================
void TextBox::Reset()
{
	text.clear();
	if(scrollbar)
	{
		scrollbar->total = 8;
		scrollbar->part = size.y - 8;
		scrollbar->offset = 0.f;
	}
	caretIndex = Int2(0, 0);
	caretPos = Int2(0, 0);
	lastYMove = -1;
	selectStartIndex = NOT_SELECTED;
}

//=================================================================================================
void TextBox::UpdateScrollbar()
{
	Int2 textSize = layout->font->CalculateSize(text, size.x - 8);
	scrollbar->total = textSize.y;
}

//=================================================================================================
void TextBox::UpdateSize(const Int2& newPos, const Int2& newSize)
{
	globalPos = pos = newPos;
	size = newSize;

	scrollbar->globalPos = scrollbar->pos = Int2(globalPos.x + size.x - 16, globalPos.y);
	scrollbar->size = Int2(16, size.y);
	scrollbar->offset = 0.f;
	scrollbar->part = size.y - 8;

	UpdateScrollbar();
}

//=================================================================================================
void TextBox::GetCaretPos(const Int2& inPos, Int2& outIndex, Int2& outPos, uint* charIndex)
{
	const Int2 realSizeWithoutPad = realSize - Int2(padding, padding) * 2;
	uint index;
	Int2 index2;
	Rect rect;
	float uv;

	if(!scrollbar)
	{
		int localX = inPos.x - globalPos.x - padding + offset;
		if(localX < 0)
		{
			outIndex = Int2(0, 0);
			outPos = Int2(0, 0);
			return;
		}

		if(localX > realSizeWithoutPad.x + offset)
			localX = realSizeWithoutPad.x + offset;
		else if(localX < offset)
			localX = offset;

		layout->font->HitTest(text, realSizeWithoutPad.x, DTF_SINGLELINE | DTF_VCENTER, Int2(localX, 0), index, index2, rect, uv, &fontLines);
	}
	else
	{
		int offsety = (int)scrollbar->offset;
		int localX = inPos.x - globalPos.x - padding;
		int localY = inPos.y - globalPos.y - padding + offsety;
		if(localX < 0)
			localX = 0;
		if(localY < 0)
			localY = 0;

		if(localX > realSizeWithoutPad.x)
			localX = realSizeWithoutPad.x;
		if(localY > realSizeWithoutPad.y + offsety)
			localY = realSizeWithoutPad.y + offsety;

		layout->font->HitTest(text, realSizeWithoutPad.x, DTF_LEFT, Int2(localX, localY), index, index2, rect, uv, &fontLines);
	}

	if(uv >= 0.5f)
	{
		outIndex = Int2(index2.x + 1, index2.y);
		outPos = Int2(rect.p2.x, rect.p1.y);
	}
	else
	{
		outIndex = index2;
		outPos = rect.p1;
	}

	if(charIndex)
		*charIndex = index;
}

//=================================================================================================
void TextBox::CalculateSelection(const Int2& newIndex, const Int2& newPos)
{
	if(selectStartIndex == NOT_SELECTED)
	{
		CalculateSelection(newIndex, newPos, caretIndex, caretPos);
		selectFixedIndex = caretIndex;
	}
	else if(selectFixedIndex == selectStartIndex)
		CalculateSelection(newIndex, newPos, selectStartIndex, selectStartPos);
	else
		CalculateSelection(newIndex, newPos, selectEndIndex, selectEndPos);
}

//=================================================================================================
void TextBox::CalculateSelection(Int2 index1, Int2 pos1, Int2 index2, Int2 pos2)
{
	if(index1.y < index2.y || (index1.x < index2.x && index1.y == index2.y))
	{
		selectStartIndex = index1;
		selectStartPos = pos1;
		selectEndIndex = index2;
		selectEndPos = pos2;
	}
	else
	{
		selectStartIndex = index2;
		selectStartPos = pos2;
		selectEndIndex = index1;
		selectEndPos = pos1;
	}
}

//=================================================================================================
void TextBox::DeleteSelection()
{
	uint start = ToRawIndex(selectStartIndex);
	uint end = ToRawIndex(selectEndIndex);
	text.erase(start, end - start);
	caretIndex = selectStartIndex;
	caretPos = selectStartPos;
	caretBlink = 0.f;
	selectStartIndex = NOT_SELECTED;
	UpdateFontLines();
	OnTextChanged();
}

//=================================================================================================
Int2 TextBox::IndexToPos(const Int2& index)
{
	const Int2 realSizeWithoutPad = realSize - Int2(padding, padding) * 2;
	int flags;
	if(scrollbar)
		flags = DTF_LEFT;
	else
		flags = DTF_SINGLELINE | DTF_VCENTER;
	return layout->font->IndexToPos(fontLines, index, text, realSizeWithoutPad.x, flags);
}

//=================================================================================================
void TextBox::SetText(Cstring newText)
{
	if(newText)
		text = newText;
	else
		text.clear();
	selectStartIndex = NOT_SELECTED;
	offset = 0;
	UpdateFontLines();
}

//=================================================================================================
void TextBox::CalculateOffset(bool center)
{
	if(!scrollbar)
	{
		const int realPos = caretPos.x - offset;
		const int realSize = size.x - padding * 2;
		const int totalWidth = layout->font->CalculateSize(text).x;
		if(realPos < 0 || realPos >= realSize)
		{
			if(center)
				offset = caretPos.x - realSize / 2;
			else if(realPos < 0)
				offset = caretPos.x;
			else
				offset = caretPos.x - realSize;
		}

		int maxOffset = totalWidth - realSize;
		if(offset > maxOffset)
			offset = maxOffset;
		if(offset < 0)
			offset = 0;
	}
	else
	{
		const int realSize = size.y - padding * 2;
		int offsety = (int)scrollbar->offset;
		int localY = caretPos.y - offsety;
		if(localY < 0)
			scrollbar->offset = (float)caretPos.y;
		else if(localY + layout->font->height > realSize)
			scrollbar->offset = (float)(caretPos.y + layout->font->height - realSize);
	}
}

//=================================================================================================
void TextBox::SelectAll()
{
	SetFocus();
	if(text.empty())
	{
		caretIndex = Int2(0, 0);
		caretPos = Int2(0, 0);
		selectStartIndex = NOT_SELECTED;
	}
	else
	{
		caretIndex = Int2(fontLines.back().count, fontLines.size());
		caretPos = Int2(fontLines.back().width, fontLines.size() * layout->font->height);
		selectStartIndex = Int2(0, 0);
		selectEndIndex = caretIndex;
		selectStartPos = Int2(0, 0);
		selectEndPos = caretPos;
		selectFixedIndex = Int2(0, 0);
	}
	lastYMove = -1;
}

//=================================================================================================
uint TextBox::ToRawIndex(const Int2& index)
{
	return layout->font->ToRawIndex(fontLines, index);
}

//=================================================================================================
void TextBox::UpdateFontLines()
{
	if(!initialized)
		return;
	int flags;
	if(multiline)
		flags = DTF_LEFT;
	else
		flags = DTF_SINGLELINE | DTF_VCENTER;

	bool numMin = requireScrollbar;
	int sizeX = size.x - padding * 2;
	if(numMin)
		sizeX -= 15;
	uint maxWidth = layout->font->PrecalculateFontLines(fontLines, text, sizeX, flags);
	uint height = fontLines.size() * layout->font->height;

	if(!scrollbar)
	{
		realSize = size;
		return;
	}

	textSize = Int2(maxWidth, height);
	scrollbar->UpdateTotal(textSize.y);
	requireScrollbar = scrollbar->IsRequired();
	if(numMin != requireScrollbar)
		UpdateFontLines();
	else
	{
		if(requireScrollbar)
			realSize = Int2(size.x - 15, size.y);
		else
			realSize = size;
	}
}
