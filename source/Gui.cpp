#include "Pch.h"
#include "Gui.h"

#include "Container.h"
#include "DialogBox.h"
#include "Engine.h"
#include "FontLoader.h"
#include "GuiRect.h"
#include "GuiShader.h"
#include "Input.h"
#include "Layout.h"
#include "Render.h"
#include "ResourceManager.h"
#include "WindowsIncludes.h"

Gui* app::gui;

//=================================================================================================
Gui::Gui() : cursorMode(CURSOR_NORMAL), focusedCtrl(nullptr), masterLayout(nullptr), layout(nullptr), overlay(nullptr), grayscale(false), shader(nullptr),
fontLoader(nullptr), lastClick(Key::LeftButton), lastClickTimer(1.f), clipRect(nullptr)
{
}

//=================================================================================================
Gui::~Gui()
{
	DeleteElements(createdDialogs);
	DeleteElements(registeredControls);
	delete masterLayout;
	delete layer;
	delete dialogLayer;
	delete fontLoader;
}

//=================================================================================================
void Gui::Init()
{
	Control::input = app::input;
	Control::gui = this;
	wndSize = app::engine->GetClientSize();
	cursorPos = wndSize / 2;

	layer = new Container;
	layer->autoFocus = true;
	dialogLayer = new Container;
	dialogLayer->focusTop = true;

	app::render->RegisterShader(shader = new GuiShader);

	fontLoader = new FontLoader;
}

//=================================================================================================
void Gui::SetText(cstring ok, cstring yes, cstring no, cstring cancel)
{
	txOk = ok;
	txYes = yes;
	txNo = no;
	txCancel = cancel;
}

//=================================================================================================
bool Gui::AddFont(cstring filename)
{
	cstring path = Format("data/fonts/%s", filename);
	int result = AddFontResourceExA(path, FR_PRIVATE, nullptr);
	if(result == 0)
	{
		Error("Failed to load font '%s' (%d)!", filename, GetLastError());
		return false;
	}
	else
	{
		Info("Added font resource '%s'.", filename);
		return true;
	}
}

//=================================================================================================
Font* Gui::GetFont(cstring name, int size, int weight, int outline)
{
	assert(name && size > 0 && InRange(weight, 1, 9) && outline >= 0);

	string res_name = Format("%s;%d;%d;%d", name, size, weight, outline);
	Font* existing_font = app::resMgr->TryGet<Font>(res_name);
	if(existing_font)
		return existing_font;

	Font* font = fontLoader->Load(name, size, weight, outline);
	font->type = ResourceType::Font;
	font->state = ResourceState::Loaded;
	font->path = res_name;
	font->filename = font->path.c_str();
	app::resMgr->AddResource(font);

	return font;
}

//=================================================================================================
// Draw text - rewritten from TFQ
bool Gui::DrawText(Font* font, Cstring str, uint flags, Color color, const Rect& rect, const Rect* clipping, vector<Hitbox>* hitboxes,
	int* hitboxCounter, const vector<TextLine>* lines)
{
	assert(font);

	uint line_begin, line_end, line_index = 0;
	int line_width, width = rect.SizeX();
	cstring text = str;
	uint text_end = strlen(str);
	bool bottom_clip = false;

	DrawLineContext ctx;
	ctx.font = font;
	ctx.text = text;
	ctx.v = vBuf;
	ctx.v2 = (IsSet(flags, DTF_OUTLINE) && font->texOutline) ? vBuf2 : nullptr;
	ctx.inBuffer = 0;
	ctx.inBuffer2 = 0;
	ctx.parseSpecial = IsSet(flags, DTF_PARSE_SPECIAL);
	ctx.scale = Vec2::One;
	ctx.defColor = color;
	ctx.currentColor = ctx.defColor;
	if(hitboxes)
	{
		ctx.hc = &tmpHitboxContext;
		ctx.hc->hitbox = hitboxes;
		ctx.hc->counter = (hitboxCounter ? *hitboxCounter : 0);
		ctx.hc->open = HitboxOpen::No;
	}
	else
		ctx.hc = nullptr;

	if(!IsSet(flags, DTF_VCENTER | DTF_BOTTOM))
	{
		int y = rect.Top();

		if(!lines)
		{
			// tekst pionowo po œrodku lub na dole
			while(font->SplitLine(line_begin, line_end, line_width, line_index, text, text_end, flags, width))
			{
				// pocz¹tkowa pozycja x w tej linijce
				int x;
				if(IsSet(flags, DTF_CENTER))
					x = rect.Left() + (width - line_width) / 2;
				else if(IsSet(flags, DTF_RIGHT))
					x = rect.Right() - line_width;
				else
					x = rect.Left();

				int clip_result = (clipping ? Clip(x, y, line_width, font->height, clipping) : 0);

				// znaki w tej linijce
				if(clip_result == 0)
					DrawTextLine(ctx, line_begin, line_end, x, y, nullptr);
				else if(clip_result == 5)
					DrawTextLine(ctx, line_begin, line_end, x, y, clipping);
				else if(clip_result == 2)
				{
					// tekst jest pod widocznym regionem, przerwij rysowanie
					bottom_clip = true;
					break;
				}
				else
				{
					// pomiñ hitbox
					SkipLine(text, line_begin, line_end, ctx.hc);
				}

				// zmieñ y na kolejn¹ linijkê
				y += font->height;
			}
		}
		else
		{
			for(vector<TextLine>::const_iterator it = lines->begin(), end = lines->end(); it != end; ++it)
			{
				// pocz¹tkowa pozycja x w tej linijce
				int x;
				if(IsSet(flags, DTF_CENTER))
					x = rect.Left() + (width - it->width) / 2;
				else if(IsSet(flags, DTF_RIGHT))
					x = rect.Right() - it->width;
				else
					x = rect.Left();

				int clip_result = (clipping ? Clip(x, y, it->width, font->height, clipping) : 0);

				// znaki w tej linijce
				if(clip_result == 0)
					DrawTextLine(ctx, it->begin, it->end, x, y, nullptr);
				else if(clip_result == 5)
					DrawTextLine(ctx, it->begin, it->end, x, y, clipping);
				else if(clip_result == 2)
				{
					// tekst jest pod widocznym regionem, przerwij rysowanie
					bottom_clip = true;
					break;
				}
				else
				{
					// pomiñ hitbox
					SkipLine(text, it->begin, it->end, ctx.hc);
				}

				// zmieñ y na kolejn¹ linijkê
				y += font->height;
			}
		}
	}
	else
	{
		// tekst u góry
		if(!lines)
		{
			static vector<TextLine> lines_data;
			lines_data.clear();

			// oblicz wszystkie linijki
			while(font->SplitLine(line_begin, line_end, line_width, line_index, text, text_end, flags, width))
				lines_data.push_back(TextLine(line_begin, line_end, line_width));

			lines = &lines_data;
		}

		// pocz¹tkowa pozycja y
		int y;
		if(IsSet(flags, DTF_BOTTOM))
			y = rect.Bottom() - lines->size() * font->height;
		else
			y = rect.Top() + (rect.SizeY() - int(lines->size()) * font->height) / 2;

		for(vector<TextLine>::const_iterator it = lines->begin(), end = lines->end(); it != end; ++it)
		{
			// pocz¹tkowa pozycja x w tej linijce
			int x;
			if(IsSet(flags, DTF_CENTER))
				x = rect.Left() + (width - it->width) / 2;
			else if(IsSet(flags, DTF_RIGHT))
				x = rect.Right() - it->width;
			else
				x = rect.Left();

			int clip_result = (clipping ? Clip(x, y, it->width, font->height, clipping) : 0);

			// znaki w tej linijce
			if(clip_result == 0)
				DrawTextLine(ctx, it->begin, it->end, x, y, nullptr);
			else if(clip_result == 5)
				DrawTextLine(ctx, it->begin, it->end, x, y, clipping);
			else if(clip_result == 2)
			{
				// tekst jest pod widocznym regionem, przerwij rysowanie
				bottom_clip = true;
				break;
			}
			else if(hitboxes)
			{
				// pomiñ hitbox
				SkipLine(text, it->begin, it->end, ctx.hc);
			}

			// zmieñ y na kolejn¹ linijkê
			y += font->height;
		}
	}

	if(ctx.inBuffer2 > 0)
		shader->Draw(font->texOutline, vBuf2, ctx.inBuffer2);
	if(ctx.inBuffer > 0)
		shader->Draw(font->tex, vBuf, ctx.inBuffer);

	if(hitboxCounter)
		*hitboxCounter = ctx.hc->counter;

	return !bottom_clip;
}

//=================================================================================================
void Gui::DrawTextLine(DrawLineContext& ctx, uint line_begin, uint line_end, int x, int y, const Rect* clipping)
{
	if(ctx.v2)
		DrawTextOutline(ctx, line_begin, line_end, x, y, clipping);

	for(uint i = line_begin; i < line_end; ++i)
	{
		char c = ctx.text[i];
		if(c == '$' && ctx.parseSpecial)
		{
			++i;
			assert(i < line_end);
			c = ctx.text[i];
			if(c == 'c')
			{
				// kolor
				++i;
				assert(i < line_end);
				c = ctx.text[i];
				switch(c)
				{
				case '-':
					ctx.currentColor = ctx.defColor;
					break;
				case 'r':
					ctx.currentColor = Vec4(1, 0, 0, ctx.defColor.w);
					break;
				case 'g':
					ctx.currentColor = Vec4(0, 1, 0, ctx.defColor.w);
					break;
				case 'b':
					ctx.currentColor = Vec4(0, 0, 1, ctx.defColor.w);
					break;
				case 'y':
					ctx.currentColor = Vec4(1, 1, 0, ctx.defColor.w);
					break;
				case 'w':
					ctx.currentColor = Vec4(1, 1, 1, ctx.defColor.w);
					break;
				case 'k':
					ctx.currentColor = Vec4(0, 0, 0, ctx.defColor.w);
					break;
				case '0':
					ctx.currentColor = Vec4(0.5f, 1, 0, ctx.defColor.w);
					break;
				case '1':
					ctx.currentColor = Vec4(1, 0.5f, 0, ctx.defColor.w);
					break;
				default:
					// nieznany kolor
					assert(0);
					break;
				}

				continue;
			}
			else if(c == 'h')
			{
				++i;
				assert(i < line_end);
				c = ctx.text[i];
				if(c == '+')
				{
					// rozpocznij hitbox
					if(ctx.hc)
					{
						assert(ctx.hc->open == HitboxOpen::No);
						ctx.hc->open = HitboxOpen::Yes;
						ctx.hc->region.Left() = INT_MAX;
					}
				}
				else if(c == '-')
				{
					// zamknij hitbox
					if(ctx.hc)
					{
						assert(ctx.hc->open == HitboxOpen::Yes);
						ctx.hc->open = HitboxOpen::No;
						if(ctx.hc->region.Left() != INT_MAX)
						{
							Hitbox& h = Add1(ctx.hc->hitbox);
							h.rect = ctx.hc->region;
							h.index = ctx.hc->counter;
							h.index2 = -1;
						}
						++ctx.hc->counter;
					}
				}
				else
				{
					// nieznana opcja hitboxa
					assert(0);
				}

				continue;
			}
			else if(c == 'g')
			{
				// group hitbox
				// $g+123[,123];
				// $g-
				++i;
				assert(i < line_end);
				c = ctx.text[i];
				if(c == '+')
				{
					// start group hitbox
					int index, index2;
					++i;
					assert(i < line_end);
					if(ctx.font->ParseGroupIndex(ctx.text, line_end, i, index, index2) && ctx.hc)
					{
						assert(ctx.hc->open == HitboxOpen::No);
						ctx.hc->open = HitboxOpen::Group;
						ctx.hc->region.Left() = INT_MAX;
						ctx.hc->groupIndex = index;
						ctx.hc->groupIndex2 = index2;
					}
				}
				else if(c == '-')
				{
					// close group hitbox
					if(ctx.hc)
					{
						assert(ctx.hc->open == HitboxOpen::Group);
						ctx.hc->open = HitboxOpen::No;
						if(ctx.hc->region.Left() != INT_MAX)
						{
							Hitbox& h = Add1(ctx.hc->hitbox);
							h.rect = ctx.hc->region;
							h.index = ctx.hc->groupIndex;
							h.index2 = ctx.hc->groupIndex2;
						}
					}
				}
				else
				{
					// invalid format
					assert(0);
				}

				continue;
			}
			else if(c == '$')
			{
				// dwa znaki $$ to $
			}
			else
			{
				// nieznana opcja
				assert(0);
			}
		}

		Font::Glyph& g = ctx.font->glyph[byte(c)];
		Int2 glyph_size = Int2(g.width, ctx.font->height) * ctx.scale;

		int clip_result = (clipping ? Clip(x, y, glyph_size.x, glyph_size.y, clipping) : 0);

		if(clip_result == 0)
		{
			// dodaj znak do bufora
			ctx.v->pos = Vec2(float(x), float(y));
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = g.uv.LeftTop();
			++ctx.v;

			ctx.v->pos = Vec2(float(x + glyph_size.x), float(y));
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = g.uv.RightTop();
			++ctx.v;

			ctx.v->pos = Vec2(float(x), float(y + glyph_size.y));
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = g.uv.LeftBottom();
			++ctx.v;

			ctx.v->pos = Vec2(float(x + glyph_size.x), float(y));
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = g.uv.RightTop();
			++ctx.v;

			ctx.v->pos = Vec2(float(x + glyph_size.x), float(y + glyph_size.y));
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = g.uv.RightBottom();
			++ctx.v;

			ctx.v->pos = Vec2(float(x), float(y + glyph_size.y));
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = g.uv.LeftBottom();
			++ctx.v;

			if(ctx.hc && ctx.hc->open != HitboxOpen::No)
			{
				Rect r_clip = Rect::Create(Int2(x, y), glyph_size);
				if(ctx.hc->region.Left() == INT_MAX)
					ctx.hc->region = r_clip;
				else
					ctx.hc->region.Resize(r_clip);
			}

			++ctx.inBuffer;
		}
		else if(clip_result == 5)
		{
			// przytnij znak
			Box2d orig_pos = Box2d::Create(Int2(x, y), glyph_size);
			Box2d clip_pos(float(max(x, clipping->Left())), float(max(y, clipping->Top())),
				float(min(x + glyph_size.x, clipping->Right())), float(min(y + glyph_size.y, clipping->Bottom())));
			Vec2 orig_size = orig_pos.Size();
			Vec2 clip_size = clip_pos.Size();
			Vec2 s(clip_size.x / orig_size.x, clip_size.y / orig_size.y);
			Vec2 shift = clip_pos.v1 - orig_pos.v1;
			shift.x /= orig_size.x;
			shift.y /= orig_size.y;
			Vec2 uv_size = g.uv.Size();
			Box2d clip_uv(g.uv.v1 + Vec2(shift.x * uv_size.x, shift.y * uv_size.y));
			clip_uv.v2 += Vec2(uv_size.x * s.x, uv_size.y * s.y);

			// dodaj znak do bufora
			ctx.v->pos = clip_pos.LeftTop();
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = clip_uv.LeftTop();
			++ctx.v;

			ctx.v->pos = clip_pos.RightTop();
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = clip_uv.RightTop();
			++ctx.v;

			ctx.v->pos = clip_pos.LeftBottom();
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = clip_uv.LeftBottom();
			++ctx.v;

			ctx.v->pos = clip_pos.RightTop();
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = clip_uv.RightTop();
			++ctx.v;

			ctx.v->pos = clip_pos.RightBottom();
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = clip_uv.RightBottom();
			++ctx.v;

			ctx.v->pos = clip_pos.LeftBottom();
			ctx.v->color = ctx.currentColor;
			ctx.v->tex = clip_uv.LeftBottom();
			++ctx.v;

			if(ctx.hc && ctx.hc->open != HitboxOpen::No)
			{
				Rect r_clip(clip_pos);
				if(ctx.hc->region.Left() == INT_MAX)
					ctx.hc->region = r_clip;
				else
					ctx.hc->region.Resize(r_clip);
			}

			++ctx.inBuffer;
		}
		else if(clip_result == 3)
		{
			// tekst jest ju¿ poza regionem z prawej, mo¿na przerwaæ
			break;
		}

		x += glyph_size.x;
		if(ctx.inBuffer == 256)
		{
			shader->Draw(ctx.font->tex, vBuf, 256);
			ctx.v = vBuf;
			ctx.inBuffer = 0;
		}
	}

	// zamknij region
	if(ctx.hc && ctx.hc->open != HitboxOpen::No && ctx.hc->region.Left() != INT_MAX)
	{
		Hitbox& h = Add1(ctx.hc->hitbox);
		h.rect = ctx.hc->region;
		if(ctx.hc->open == HitboxOpen::Yes)
			h.index = ctx.hc->counter;
		else
		{
			h.index = ctx.hc->groupIndex;
			h.index2 = ctx.hc->groupIndex2;
		}
		ctx.hc->region.Left() = INT_MAX;
	}
}

//=================================================================================================
void Gui::DrawTextOutline(DrawLineContext& ctx, uint line_begin, uint line_end, int x, int y, const Rect* clipping)
{
	// scale is TODO here
	assert(ctx.scale == Vec2::One);

	const Vec4 col(0, 0, 0, ctx.defColor.w);
	const float outline = (float)ctx.font->outline;
	const Vec2& osh = ctx.font->outlineShift;

	for(uint i = line_begin; i < line_end; ++i)
	{
		char c = ctx.text[i];
		if(c == '$' && ctx.parseSpecial)
		{
			++i;
			assert(i < line_end);
			c = ctx.text[i];
			if(c == 'c')
			{
				// kolor
				++i;
				assert(i < line_end);
				continue;
			}
			else if(c == 'h')
			{
				++i;
				assert(i < line_end);
				continue;
			}
			else if(c == 'g')
			{
				++i;
				assert(i < line_end);
				c = ctx.text[i];
				if(c == '+')
				{
					++i;
					assert(i < line_end);
					int tmp;
					ctx.font->ParseGroupIndex(ctx.text, line_end, i, tmp, tmp);
				}
				else if(c == '-')
					continue;
				else
				{
					// invalid group format
					assert(0);
				}
			}
			else if(c == '$')
			{
				// dwa znaki $$ to $
			}
			else
			{
				// nieznana opcja
				assert(0);
			}
		}

		Font::Glyph& g = ctx.font->glyph[byte(c)];
		const Box2d uv = Box2d(g.uv.v1.x - osh.x, g.uv.v1.y - osh.y, g.uv.v2.x + osh.x, g.uv.v2.y + osh.y);
		const int clip_result = (clipping ? Clip(x, y, g.width, ctx.font->height, clipping) : 0);

		if(clip_result == 0)
		{
			// dodaj znak do bufora
			ctx.v2->pos = Vec2(float(x) - outline, float(y) - outline);
			ctx.v2->color = col;
			ctx.v2->tex = uv.LeftTop();
			++ctx.v2;

			ctx.v2->pos = Vec2(float(x + g.width) + outline, float(y) - outline);
			ctx.v2->color = col;
			ctx.v2->tex = uv.RightTop();
			++ctx.v2;

			ctx.v2->pos = Vec2(float(x) - outline, float(y + ctx.font->height) + outline);
			ctx.v2->color = col;
			ctx.v2->tex = uv.LeftBottom();
			++ctx.v2;

			ctx.v2->pos = Vec2(float(x + g.width) + outline, float(y) - outline);
			ctx.v2->color = col;
			ctx.v2->tex = uv.RightTop();
			++ctx.v2;

			ctx.v2->pos = Vec2(float(x + g.width) + outline, float(y + ctx.font->height) + outline);
			ctx.v2->color = col;
			ctx.v2->tex = uv.RightBottom();
			++ctx.v2;

			ctx.v2->pos = Vec2(float(x) - outline, float(y + ctx.font->height) + outline);
			ctx.v2->color = col;
			ctx.v2->tex = uv.LeftBottom();
			++ctx.v2;

			++ctx.inBuffer2;
			x += g.width;
		}
		else if(clip_result == 5)
		{
			// przytnij znak
			Box2d orig_pos(float(x) - outline, float(y) - outline, float(x + g.width) + outline, float(y + ctx.font->height) + outline);
			Box2d clip_pos(float(max(x, clipping->Left())), float(max(y, clipping->Top())),
				float(min(x + g.width, clipping->Right())), float(min(y + ctx.font->height, clipping->Bottom())));
			Vec2 orig_size = orig_pos.Size();
			Vec2 clip_size = clip_pos.Size();
			Vec2 s(clip_size.x / orig_size.x, clip_size.y / orig_size.y);
			Vec2 shift = clip_pos.v1 - orig_pos.v1;
			shift.x /= orig_size.x;
			shift.y /= orig_size.y;
			Vec2 uv_size = uv.Size();
			Box2d clip_uv(uv.v1 + Vec2(shift.x * uv_size.x, shift.y * uv_size.y));
			clip_uv.v2 += Vec2(uv_size.x * s.x, uv_size.y * s.y);

			// dodaj znak do bufora
			ctx.v2->pos = clip_pos.LeftTop();
			ctx.v2->color = col;
			ctx.v2->tex = clip_uv.LeftTop();
			++ctx.v2;

			ctx.v2->pos = clip_pos.RightTop();
			ctx.v2->color = col;
			ctx.v2->tex = clip_uv.RightTop();
			++ctx.v2;

			ctx.v2->pos = clip_pos.LeftBottom();
			ctx.v2->color = col;
			ctx.v2->tex = clip_uv.LeftBottom();
			++ctx.v2;

			ctx.v2->pos = clip_pos.RightTop();
			ctx.v2->color = col;
			ctx.v2->tex = clip_uv.RightTop();
			++ctx.v2;

			ctx.v2->pos = clip_pos.RightBottom();
			ctx.v2->color = col;
			ctx.v2->tex = clip_uv.RightBottom();
			++ctx.v2;

			ctx.v2->pos = clip_pos.LeftBottom();
			ctx.v2->color = col;
			ctx.v2->tex = clip_uv.LeftBottom();
			++ctx.v2;

			++ctx.inBuffer2;
			x += g.width;
		}
		else if(clip_result == 3)
		{
			// tekst jest ju¿ poza regionem z prawej, mo¿na przerwaæ
			break;
		}
		else
			x += g.width;

		if(ctx.inBuffer2 == 256)
		{
			shader->Draw(ctx.font->texOutline, vBuf2, 256);
			ctx.v2 = vBuf2;
			ctx.inBuffer2 = 0;
		}
	}
}

//=================================================================================================
void Gui::Draw(bool drawLayers, bool drawDialogs)
{
	wndSize = app::engine->GetClientSize();

	if(!drawLayers && !drawDialogs)
		return;

	shader->Prepare();

	// rysowanie
	if(drawLayers)
		layer->Draw();
	if(drawDialogs)
		dialogLayer->Draw();

	// draw cursor
	if(NeedCursor())
	{
		assert(layout);
		Int2 pos = cursorPos;
		if(cursorMode == CURSOR_TEXT)
			pos -= Int2(3, 8);
		DrawSprite(layout->cursor[cursorMode], pos);
	}
}

//=================================================================================================
void Gui::Add(Control* ctrl)
{
	layer->Add(ctrl);
}

//=================================================================================================
void Gui::DrawItem(Texture* t, const Int2& itemPos, const Int2& itemSize, Color color, int corner, int size, const Box2d* clipRect)
{
	assert(t && t->IsLoaded());

	GuiRect guiRect;
	guiRect.Set(itemPos, itemSize);

	bool require_clip = false;
	if(clipRect)
	{
		int result = guiRect.RequireClip(*clipRect);
		if(result == -1)
			return;
		else if(result == 1)
			require_clip = true;
	}

	if(itemSize.x < corner || itemSize.y < corner)
	{
		if(itemSize.x == 0 || itemSize.y == 0)
			return;

		Rect r = { itemPos.x, itemPos.y, itemPos.x + itemSize.x, itemPos.y + itemSize.y };
		assert(!clipRect);
		DrawSpriteRect(t, r, color);
		return;
	}

	VGui* v = vBuf;
	const Vec4 col = color;

	/*
		0---1----------2---3
		| 1 |     2    | 3 |
		|   |          |   |
		4---5----------6---7
		|   |          |   |
		| 4 |     5    | 6 |
		|   |          |   |
		|   |          |   |
		8---9---------10--11
		|   |          |   |
		| 7 |     8    | 9 |
		12-13---------14--15
	*/

	// top left & bottom right indices for each rectangle
	int ids[9 * 2] = {
		0, 5,
		1, 6,
		2, 7,
		4, 9,
		5, 10,
		6, 11,
		8, 13,
		9, 14,
		10, 15
	};

	Vec2 ipos[16] = {
		Vec2(float(itemPos.x), float(itemPos.y)),
		Vec2(float(itemPos.x + corner), float(itemPos.y)),
		Vec2(float(itemPos.x + itemSize.x - corner), float(itemPos.y)),
		Vec2(float(itemPos.x + itemSize.x), float(itemPos.y)),

		Vec2(float(itemPos.x), float(itemPos.y + corner)),
		Vec2(float(itemPos.x + corner), float(itemPos.y + corner)),
		Vec2(float(itemPos.x + itemSize.x - corner), float(itemPos.y + corner)),
		Vec2(float(itemPos.x + itemSize.x), float(itemPos.y + corner)),

		Vec2(float(itemPos.x), float(itemPos.y + itemSize.y - corner)),
		Vec2(float(itemPos.x + corner), float(itemPos.y + itemSize.y - corner)),
		Vec2(float(itemPos.x + itemSize.x - corner), float(itemPos.y + itemSize.y - corner)),
		Vec2(float(itemPos.x + itemSize.x), float(itemPos.y + itemSize.y - corner)),

		Vec2(float(itemPos.x), float(itemPos.y + itemSize.y)),
		Vec2(float(itemPos.x + corner), float(itemPos.y + itemSize.y)),
		Vec2(float(itemPos.x + itemSize.x - corner), float(itemPos.y + itemSize.y)),
		Vec2(float(itemPos.x + itemSize.x), float(itemPos.y + itemSize.y)),
	};

	Vec2 itex[16] = {
		Vec2(0,0),
		Vec2(float(corner) / size,0),
		Vec2(float(size - corner) / size,0),
		Vec2(1,0),

		Vec2(0,float(corner) / size),
		Vec2(float(corner) / size,float(corner) / size),
		Vec2(float(size - corner) / size,float(corner) / size),
		Vec2(1,float(corner) / size),

		Vec2(0,float(size - corner) / size),
		Vec2(float(corner) / size,float(size - corner) / size),
		Vec2(float(size - corner) / size,float(size - corner) / size),
		Vec2(1,float(size - corner) / size),

		Vec2(0,1),
		Vec2(float(corner) / size,1),
		Vec2(float(size - corner) / size,1),
		Vec2(1,1),
	};

	int inBuffer;
	if(require_clip)
	{
		inBuffer = 0;
		for(int i = 0; i < 9; ++i)
		{
			int index1 = ids[i * 2 + 0];
			int index2 = ids[i * 2 + 1];
			guiRect.Set(ipos[index1], ipos[index2], itex[index1], itex[index2]);
			if(guiRect.Clip(*clipRect))
			{
				guiRect.Populate(v, col);
				++inBuffer;
			}
		}
	}
	else
	{
		for(int i = 0; i < 9; ++i)
		{
			int index1 = ids[i * 2 + 0];
			int index2 = ids[i * 2 + 1];
			guiRect.Set(ipos[index1], ipos[index2], itex[index1], itex[index2]);
			guiRect.Populate(v, col);
		}
		inBuffer = 9;
	}

	shader->Draw(t->tex, vBuf, inBuffer);
}

//=================================================================================================
void Gui::Update(float dt, float mouseSpeed)
{
	// update cursor
	cursorMode = CURSOR_NORMAL;
	prevCursorPos = cursorPos;
	if(NeedCursor() && mouseSpeed > 0)
	{
		cursorPos += app::input->GetMouseDif() * mouseSpeed;
		if(cursorPos.x < 0)
			cursorPos.x = 0;
		if(cursorPos.y < 0)
			cursorPos.y = 0;
		if(cursorPos.x >= wndSize.x)
			cursorPos.x = wndSize.x - 1;
		if(cursorPos.y >= wndSize.y)
			cursorPos.y = wndSize.y - 1;
		app::engine->SetUnlockPoint(cursorPos);
	}
	else
		app::engine->SetUnlockPoint(wndSize / 2);

	// handle double click
	for(int i = 0; i < 5; ++i)
		doubleclk[i] = false;
	bool handled = false;
	for(int i = 0; i < 5; ++i)
	{
		const Key key = Key(i + 1);
		if(app::input->Pressed(key))
		{
			if(lastClick == key && lastClickTimer <= 0.5f && abs(cursorPos.x - lastClickPos.x) <= 4 && abs(cursorPos.y - lastClickPos.y) <= 4)
			{
				doubleclk[i] = true;
				lastClickTimer = 1.f;
			}
			else
			{
				lastClick = key;
				lastClickTimer = 0.f;
				lastClickPos = cursorPos;
			}
			handled = true;
			break;
		}
	}
	if(!handled)
		lastClickTimer += dt;

	layer->focus = dialogLayer->Empty();

	if(focusedCtrl)
	{
		if(!focusedCtrl->visible)
			focusedCtrl = nullptr;
		else if(dialogLayer->Empty())
		{
			layer->dontFocus = true;
			layer->Update(dt);
			layer->dontFocus = false;
		}
		else
		{
			focusedCtrl->LostFocus();
			focusedCtrl = nullptr;
		}
	}

	if(!focusedCtrl)
	{
		dialogLayer->focus = true;
		dialogLayer->Update(dt);
		layer->Update(dt);
	}

	app::engine->SetUnlockPoint(wndSize / 2);
}

//=================================================================================================
void Gui::DrawSprite(Texture* t, const Int2& pos, Color color, const Rect* clipping)
{
	assert(t && t->IsLoaded());

	Int2 size = t->GetSize();

	int clip_result = (clipping ? Clip(pos.x, pos.y, size.x, size.y, clipping) : 0);
	if(clip_result > 0 && clip_result < 5)
		return;

	VGui* v = vBuf;
	const Vec4 col = color;

	if(clip_result == 0)
	{
		v->pos = Vec2(float(pos.x), float(pos.y));
		v->color = col;
		v->tex = Vec2(0, 0);
		++v;

		v->pos = Vec2(float(pos.x + size.x), float(pos.y));
		v->color = col;
		v->tex = Vec2(1, 0);
		++v;

		v->pos = Vec2(float(pos.x), float(pos.y + size.y));
		v->color = col;
		v->tex = Vec2(0, 1);
		++v;

		v->pos = Vec2(float(pos.x), float(pos.y + size.y));
		v->color = col;
		v->tex = Vec2(0, 1);
		++v;

		v->pos = Vec2(float(pos.x + size.x), float(pos.y));
		v->color = col;
		v->tex = Vec2(1, 0);
		++v;

		v->pos = Vec2(float(pos.x + size.x), float(pos.y + size.y));
		v->color = col;
		v->tex = Vec2(1, 1);
		++v;
	}
	else
	{
		Box2d orig_pos(float(pos.x), float(pos.y), float(pos.x + size.x), float(pos.y + size.y));
		Box2d clip_pos(float(max(pos.x, clipping->Left())), float(max(pos.y, clipping->Top())),
			float(min(pos.x + (int)size.x, clipping->Right())), float(min(pos.y + (int)size.y, clipping->Bottom())));
		Vec2 orig_size = orig_pos.Size();
		Vec2 clip_size = clip_pos.Size();
		Vec2 s(clip_size.x / orig_size.x, clip_size.y / orig_size.y);
		Vec2 shift = clip_pos.v1 - orig_pos.v1;
		shift.x /= orig_size.x;
		shift.y /= orig_size.y;
		Box2d clip_uv(Vec2(shift.x, shift.y));
		clip_uv.v2 += Vec2(s.x, s.y);

		v->pos = clip_pos.LeftTop();
		v->color = col;
		v->tex = clip_uv.LeftTop();
		++v;

		v->pos = clip_pos.RightTop();
		v->color = col;
		v->tex = clip_uv.RightTop();
		++v;

		v->pos = clip_pos.LeftBottom();
		v->color = col;
		v->tex = clip_uv.LeftBottom();
		++v;

		v->pos = clip_pos.LeftBottom();
		v->color = col;
		v->tex = clip_uv.LeftBottom();
		++v;

		v->pos = clip_pos.RightTop();
		v->color = col;
		v->tex = clip_uv.RightTop();
		++v;

		v->pos = clip_pos.RightBottom();
		v->color = col;
		v->tex = clip_uv.RightBottom();
		++v;
	}

	shader->Draw(t->tex, vBuf, 1);
}

//=================================================================================================
/*
Przycinanie tekstu do wybranego regionu, zwraca:
0 - tekst w ca³oœci w regionie
1 - tekst nad regionem
2 - tekst pod regionem
3 - tekst poza regionem z prawej
4 - tekst poza regionem z lewej
5 - wymaga czêœciowego clippingu, czêœciowo w regionie
*/
int Gui::Clip(int x, int y, int w, int h, const Rect* clipping)
{
	if(x >= clipping->Left() && y >= clipping->Top() && x + w < clipping->Right() && y + h < clipping->Bottom())
	{
		// tekst w ca³oœci w regionie
		return 0;
	}
	else if(y + h < clipping->Top())
	{
		// tekst nad regionem
		return 1;
	}
	else if(y > clipping->Bottom())
	{
		// tekst pod regionem
		return 2;
	}
	else if(x > clipping->Right())
	{
		// tekst poza regionem z prawej
		return 3;
	}
	else if(x + w < clipping->Left())
	{
		// tekst poza regionem z lewej
		return 4;
	}
	else
	{
		// wymaga czêœciowego clippingu
		return 5;
	}
}

//=================================================================================================
void Gui::SkipLine(cstring text, uint line_begin, uint line_end, HitboxContext* hc)
{
	for(uint i = line_begin; i < line_end; ++i)
	{
		char c = text[i];
		if(c == '$')
		{
			++i;
			c = text[i];
			if(c == 'h')
			{
				++i;
				c = text[i];
				if(c == '+')
				{
					assert(hc->open == HitboxOpen::No);
					hc->open = HitboxOpen::Yes;
				}
				else if(c == '-')
				{
					assert(hc->open == HitboxOpen::Yes);
					hc->open = HitboxOpen::No;
					++hc->counter;
				}
				else
					assert(0);
			}
			else if(c == 'g')
			{
				++i;
				c = text[i];
				if(c == '+')
				{
					assert(hc->open == HitboxOpen::No);
					hc->open = HitboxOpen::Group;
					int tmp;
					++i;
					Font::ParseGroupIndex(text, line_end, i, tmp, tmp);
				}
				else if(c == '-')
				{
					assert(hc->open == HitboxOpen::Group);
					hc->open = HitboxOpen::No;
				}
				else
					assert(0);
			}
		}
	}
}

//=================================================================================================
DialogBox* Gui::ShowDialog(const DialogInfo& info)
{
	assert(!(info.haveTick && info.img)); // TODO

	DialogBox* d;
	int extra_limit = 0;
	Int2 min_size(0, 0);

	// create dialog
	if(info.haveTick)
		d = new DialogWithCheckbox(info);
	else if(info.img)
	{
		DialogWithImage* dwi = new DialogWithImage(info);
		Int2 size = dwi->GetImageSize();
		extra_limit = size.x + 8;
		min_size.y = size.y;
		d = dwi;
	}
	else
		d = new DialogBox(info);
	createdDialogs.push_back(d);

	// calculate size
	Font* font = d->layout->font;
	Int2 text_size;
	if(!info.autoWrap)
		text_size = font->CalculateSize(info.text);
	else
		text_size = font->CalculateSizeWrap(info.text, wndSize, 24 + 32 + extra_limit);
	d->size = text_size + Int2(24 + extra_limit, 24 + max(0, min_size.y - text_size.y));

	// set buttons
	if(info.type == DIALOG_OK)
	{
		Button& bt = Add1(d->bts);
		bt.text = txOk;
		bt.id = GuiEvent_Custom + BUTTON_OK;
		bt.size = font->CalculateSize(bt.text) + Int2(24, 24);
		bt.parent = d;

		min_size.x = bt.size.x + 24;
	}
	else
	{
		d->bts.resize(2);
		Button& bt1 = d->bts[0],
			& bt2 = d->bts[1];

		if(info.customNames)
		{
			bt1.text = (info.customNames[0] ? info.customNames[0] : txYes);
			bt2.text = (info.customNames[1] ? info.customNames[1] : txNo);
		}
		else
		{
			bt1.text = txYes;
			bt2.text = txNo;
		}

		bt1.id = GuiEvent_Custom + BUTTON_YES;
		bt1.size = font->CalculateSize(bt1.text) + Int2(24, 24);
		bt1.parent = d;

		bt2.id = GuiEvent_Custom + BUTTON_NO;
		bt2.size = font->CalculateSize(bt2.text) + Int2(24, 24);
		bt2.parent = d;

		bt1.size = bt2.size = Int2::Max(bt1.size, bt2.size);
		min_size.x = bt1.size.x * 2 + 24 + 16;
	}

	// powiêksz rozmiar okna o przyciski
	if(d->size.x < min_size.x)
		d->size.x = min_size.x;
	d->size.y += d->bts[0].size.y + 8;

	// checkbox
	if(info.haveTick)
	{
		d->size.y += 32;
		DialogWithCheckbox* dwc = static_cast<DialogWithCheckbox*>(d);
		dwc->checkbox.btSize = Int2(32, 32);
		dwc->checkbox.checked = info.ticked;
		dwc->checkbox.id = GuiEvent_Custom + BUTTON_CHECKED;
		dwc->checkbox.parent = dwc;
		dwc->checkbox.text = info.tickText;
		dwc->checkbox.pos = Int2(12, 40);
		dwc->checkbox.size = Int2(d->size.x - 24, 32);
	}

	// ustaw przyciski
	if(d->bts.size() == 1)
	{
		Button& bt = d->bts[0];
		bt.pos.x = (d->size.x - bt.size.x) / 2;
		bt.pos.y = d->size.y - 8 - bt.size.y;
	}
	else
	{
		Button& bt1 = d->bts[0],
			& bt2 = d->bts[1];
		bt1.pos.y = bt2.pos.y = d->size.y - 8 - bt1.size.y;
		bt1.pos.x = 12;
		bt2.pos.x = d->size.x - bt2.size.x - 12;
	}

	// dodaj
	d->needDelete = true;
	d->Setup(text_size);
	ShowDialog(d);

	return d;
}

//=================================================================================================
void Gui::ShowDialog(DialogBox* d)
{
	d->visible = true;
	d->Event(GuiEvent_Show);

	if(dialogLayer->Empty())
	{
		// nie ma ¿adnych innych dialogów, aktywuj
		dialogLayer->Add(d);
		d->focus = true;
		d->Event(GuiEvent_GainFocus);
	}
	else if(d->order == DialogOrder::TopMost)
	{
		// dezaktywuj aktualny i aktywuj nowy
		Control* prev_d = dialogLayer->Top();
		prev_d->focus = false;
		prev_d->Event(GuiEvent_LostFocus);
		dialogLayer->Add(d);
		d->focus = true;
		d->Event(GuiEvent_GainFocus);
	}
	else
	{
		// szukaj pierwszego dialogu który jest wy¿ej ni¿ ten
		DialogOrder above_order = DialogOrder((int)d->order + 1);
		vector<DialogBox*>& ctrls = (vector<DialogBox*>&)dialogLayer->GetControls();
		vector<DialogBox*>::iterator first_above = ctrls.end();
		for(vector<DialogBox*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			if((*it)->order >= above_order)
			{
				first_above = it;
				break;
			}
		}

		if(first_above == ctrls.end())
		{
			// brak nadrzêdnego dialogu, dezaktywuj aktualny i aktywuj nowy
			Control* prev_d = dialogLayer->Top();
			prev_d->focus = false;
			prev_d->Event(GuiEvent_LostFocus);
			dialogLayer->Add(d);
			d->focus = true;
			d->Event(GuiEvent_GainFocus);
		}
		else
		{
			// jest nadrzêdny dialog, dodaj przed nim i nie aktywuj
			ctrls.insert(first_above, d);
		}
	}

	dialogLayer->insideLoop = false;
}

//=================================================================================================
bool Gui::CloseDialog(DialogBox* d)
{
	assert(d);

	if(dialogLayer->Empty() || !HaveDialog(d))
		return false;

	Control* prev_top = dialogLayer->Top();
	CloseDialogInternal(d);
	if(!dialogLayer->Empty() && prev_top != dialogLayer->Top())
	{
		Control* next_d = dialogLayer->Top();
		next_d->focus = true;
		next_d->Event(GuiEvent_GainFocus);
	}

	return true;
}

//=================================================================================================
void Gui::CloseDialogInternal(DialogBox* d)
{
	assert(d);

	d->Event(GuiEvent_Close);
	d->visible = false;
	dialogLayer->Remove(d);

	if(!dialogLayer->Empty())
	{
		vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialogLayer->GetControls();
		static vector<DialogBox*> to_remove;
		for(vector<DialogBox*>::iterator it = dialogs.begin(), end = dialogs.end(); it != end; ++it)
		{
			if((*it)->parent == d)
				to_remove.push_back(*it);
		}
		if(!to_remove.empty())
		{
			for(vector<DialogBox*>::iterator it = to_remove.begin(), end = to_remove.end(); it != end; ++it)
				CloseDialogInternal(*it);
			to_remove.clear();
		}
	}

	if(d->needDelete)
	{
		RemoveElement(createdDialogs, d);
		delete d;
	}
}

//=================================================================================================
bool Gui::HaveTopDialog(cstring name) const
{
	assert(name);

	if(dialogLayer->Empty())
		return false;

	DialogBox* d = static_cast<DialogBox*>(dialogLayer->Top());
	return d->name == name;
}

//=================================================================================================
bool Gui::HaveDialog() const
{
	return !dialogLayer->Empty();
}

//=================================================================================================
void Gui::DrawSpriteFull(Texture* t, const Color color)
{
	assert(t && t->IsLoaded());

	VGui* v = vBuf;
	const Vec4 col = color;

	v->pos = Vec2(0, 0);
	v->color = col;
	v->tex = Vec2(0, 0);
	++v;

	v->pos = Vec2(float(wndSize.x), 0);
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = Vec2(0, float(wndSize.y));
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = Vec2(0, float(wndSize.y));
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = Vec2(float(wndSize.x), 0);
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = Vec2(float(wndSize.x), float(wndSize.y));
	v->color = col;
	v->tex = Vec2(1, 1);
	++v;

	shader->Draw(t->tex, vBuf, 1);
}

//=================================================================================================
void Gui::DrawSpriteFullWrap(Texture* t, Color color)
{
	Rect rect(Int2::Zero, wndSize);
	shader->SetWrap(true);
	DrawSpriteRectPart(t, rect, rect, color);
	shader->SetWrap(false);
}

//=================================================================================================
void Gui::OnChar(char c)
{
	if((c != (char)Key::Backspace && c != (char)Key::Enter && byte(c) < 0x20) || c == '`')
		return;

	for(vector<OnCharHandler*>::iterator it = onChar.begin(), end = onChar.end(); it != end; ++it)
	{
		Control* ctrl = dynamic_cast<Control*>(*it);
		if(ctrl->visible)
			(*it)->OnChar(c);
	}
}

//=================================================================================================
void Gui::SimpleDialog(cstring text, Control* parent, cstring name)
{
	DialogInfo di;
	di.event = nullptr;
	di.name = name;
	di.parent = parent;
	di.pause = false;
	di.text = text;
	di.order = DialogBox::GetOrder(parent);
	di.type = DIALOG_OK;

	ShowDialog(di);
}

//=================================================================================================
void Gui::DrawSpriteRect(Texture* t, const Rect& rect, Color color)
{
	assert(t && t->IsLoaded());

	VGui* v = vBuf;
	const Vec4 col = color;

	v->pos = Vec2(float(rect.Left()), float(rect.Top()));
	v->color = col;
	v->tex = Vec2(0, 0);
	++v;

	v->pos = Vec2(float(rect.Right()), float(rect.Top()));
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = Vec2(float(rect.Left()), float(rect.Bottom()));
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = Vec2(float(rect.Left()), float(rect.Bottom()));
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = Vec2(float(rect.Right()), float(rect.Top()));
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = Vec2(float(rect.Right()), float(rect.Bottom()));
	v->color = col;
	v->tex = Vec2(1, 1);
	++v;

	shader->Draw(t->tex, vBuf, 1);
}

//=================================================================================================
bool Gui::HaveDialog(cstring name)
{
	assert(name);
	vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialogLayer->GetControls();
	for(DialogBox* dialog : dialogs)
	{
		if(dialog->name == name)
			return true;
	}
	return false;
}

//=================================================================================================
bool Gui::HaveDialog(DialogBox* dialog)
{
	assert(dialog);
	vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialogLayer->GetControls();
	for(auto d : dialogs)
	{
		if(d == dialog)
			return true;
	}
	return false;
}

//=================================================================================================
bool Gui::AnythingVisible() const
{
	return !dialogLayer->Empty() || layer->AnythingVisible();
}

//=================================================================================================
void Gui::OnResize()
{
	wndSize = app::engine->GetClientSize();
	cursorPos = wndSize / 2;
	app::engine->SetUnlockPoint(cursorPos);
	layer->Event(GuiEvent_WindowResize);
	dialogLayer->Event(GuiEvent_WindowResize);
}

//=================================================================================================
void Gui::DrawSpriteRectPart(Texture* t, const Rect& rect, const Rect& part, Color color)
{
	assert(t && t->IsLoaded());

	VGui* v = vBuf;
	Int2 size = t->GetSize();
	const Vec4 col = color;
	Box2d uv(float(part.Left()) / size.x, float(part.Top()) / size.y, float(part.Right()) / size.x, float(part.Bottom()) / size.y);

	v->pos = Vec2(float(rect.Left()), float(rect.Top()));
	v->color = col;
	v->tex = uv.LeftTop();
	++v;

	v->pos = Vec2(float(rect.Right()), float(rect.Top()));
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = Vec2(float(rect.Left()), float(rect.Bottom()));
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = Vec2(float(rect.Left()), float(rect.Bottom()));
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = Vec2(float(rect.Right()), float(rect.Top()));
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = Vec2(float(rect.Right()), float(rect.Bottom()));
	v->color = col;
	v->tex = uv.RightBottom();
	++v;

	shader->Draw(t->tex, vBuf, 1);
}

//=================================================================================================
void Gui::DrawSpriteTransform(Texture* t, const Matrix& mat, Color color)
{
	assert(t && t->IsLoaded());

	Int2 size = t->GetSize();
	VGui* v = vBuf;
	const Vec4 col = color;

	Vec2 leftTop(0, 0),
		rightTop(float(size.x), 0),
		leftBottom(0, float(size.y)),
		rightBottom(float(size.x), float(size.y));

	leftTop = Vec2::Transform(leftTop, mat);
	rightTop = Vec2::Transform(rightTop, mat);
	leftBottom = Vec2::Transform(leftBottom, mat);
	rightBottom = Vec2::Transform(rightBottom, mat);

	v->pos = leftTop;
	v->color = col;
	v->tex = Vec2(0, 0);
	++v;

	v->pos = rightTop;
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = leftBottom;
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = leftBottom;
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = rightTop;
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = rightBottom;
	v->color = col;
	v->tex = Vec2(1, 1);
	++v;

	shader->Draw(t->tex, vBuf, 1);
}

//=================================================================================================
void Gui::DrawLine(const Vec2& from, const Vec2& to, Color color, float width)
{
	assert(width >= 1.f);

	VGui* v = vBuf;
	const Vec2 dir = (to - from).Normalized();
	int parts = 1;

	if(width < 2.f)
	{
		Vec4 col = color;
		col.w /= 2;
		const Vec2 dirY = dir * width * 1.5f;
		const Vec2 dirX = Vec2(dirY.y, -dirY.x);

		v->pos = from - dirY - dirX;
		v->color = col;
		v->tex = Vec2(0, 0);
		++v;

		v->pos = to + dirY - dirX;
		v->color = col;
		v->tex = Vec2(0, 1);
		++v;

		v->pos = from - dirY + dirX;
		v->color = col;
		v->tex = Vec2(1, 0);
		++v;

		v->pos = from - dirY + dirX;
		v->color = col;
		v->tex = Vec2(1, 0);
		++v;

		v->pos = to + dirY - dirX;
		v->color = col;
		v->tex = Vec2(0, 1);
		++v;

		v->pos = to + dirY + dirX;
		v->color = col;
		v->tex = Vec2(1, 1);
		++v;

		++parts;
	}

	const Vec4 col = color;
	const Vec2 dirY = dir * (width * 0.5f);
	const Vec2 dirX = Vec2(dirY.y, -dirY.x);

	v->pos = from - dirY - dirX;
	v->color = col;
	v->tex = Vec2(0, 0);
	++v;

	v->pos = to + dirY - dirX;
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = from - dirY + dirX;
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = from - dirY + dirX;
	v->color = col;
	v->tex = Vec2(1, 0);
	++v;

	v->pos = to + dirY - dirX;
	v->color = col;
	v->tex = Vec2(0, 1);
	++v;

	v->pos = to + dirY + dirX;
	v->color = col;
	v->tex = Vec2(1, 1);
	++v;

	shader->Draw(nullptr, vBuf, parts);
}

//=================================================================================================
bool Gui::NeedCursor()
{
	if(!dialogLayer->Empty())
		return true;
	else
		return layer->visible && layer->NeedCursor();
}

//=================================================================================================
bool Gui::DrawText3D(Font* font, Cstring text, uint flags, Color color, const Vec3& pos, Rect* textRect)
{
	assert(font);

	Int2 pt;
	if(!To2dPoint(pos, pt))
		return false;

	Int2 size = font->CalculateSize(text);
	Rect r = { pt.x - size.x / 2, pt.y - size.y - 4, pt.x + size.x / 2 + 1, pt.y - 4 };
	if(!IsSet(flags, DTF_DONT_DRAW))
		DrawText(font, text, flags, color, r);

	if(textRect)
		*textRect = r;

	return true;
}

//=================================================================================================
bool Gui::To2dPoint(const Vec3& pos, Int2& pt)
{
	Vec4 v4;
	Vec3::Transform(pos, mViewProj, v4);

	if(v4.z < 0)
	{
		// jest poza kamer¹
		return false;
	}

	Vec3 v3;

	// see if we are in world space already
	v3 = Vec3(v4.x, v4.y, v4.z);
	if(v4.w != 1)
	{
		if(v4.w == 0)
			v4.w = 0.00001f;
		v3 /= v4.w;
	}

	pt.x = int(v3.x * (wndSize.x / 2) + (wndSize.x / 2));
	pt.y = -int(v3.y * (wndSize.y / 2) - (wndSize.y / 2));

	return true;
}

//=================================================================================================
bool Gui::Intersect(vector<Hitbox>& hitboxes, const Int2& pt, int* index, int* index2)
{
	for(vector<Hitbox>::iterator it = hitboxes.begin(), end = hitboxes.end(); it != end; ++it)
	{
		if(it->rect.IsInside(pt))
		{
			if(index)
				*index = it->index;
			if(index2)
				*index2 = it->index2;
			return true;
		}
	}

	return false;
}

//=================================================================================================
void Gui::DrawSpriteTransformPart(Texture* t, const Matrix& mat, const Rect& part, Color color)
{
	assert(t && t->IsLoaded());

	VGui* v = vBuf;
	Int2 size = t->GetSize();
	Box2d uv(float(part.Left()) / size.x, float(part.Top() / size.y), float(part.Right()) / size.x, float(part.Bottom()) / size.y);
	const Vec4 col = color;

	Vec2 leftTop(part.LeftTop()),
		rightTop(part.RightTop()),
		leftBottom(part.LeftBottom()),
		rightBottom(part.RightBottom());

	leftTop = Vec2::Transform(leftTop, mat);
	rightTop = Vec2::Transform(rightTop, mat);
	leftBottom = Vec2::Transform(leftBottom, mat);
	rightBottom = Vec2::Transform(rightBottom, mat);

	v->pos = leftTop;
	v->color = col;
	v->tex = uv.LeftTop();
	++v;

	v->pos = rightTop;
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = leftBottom;
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = leftBottom;
	v->color = col;
	v->tex = uv.LeftBottom();
	++v;

	v->pos = rightTop;
	v->color = col;
	v->tex = uv.RightTop();
	++v;

	v->pos = rightBottom;
	v->color = col;
	v->tex = uv.RightBottom();
	++v;

	shader->Draw(t->tex, vBuf, 1);
}

//=================================================================================================
void Gui::CloseDialogs()
{
	vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialogLayer->GetControls();
	for(DialogBox* dialog : dialogs)
	{
		if(dialog->type == DIALOG_CUSTOM)
			dialog->Event(GuiEvent_Close);
		if(dialog->needDelete)
		{
			if(IsDebug())
				RemoveElementTry(createdDialogs, dialog);
			delete dialog;
		}
	}
	dialogs.clear();
	dialogLayer->insideLoop = false;
	assert(createdDialogs.empty());
	createdDialogs.clear();
}

//=================================================================================================
bool Gui::HavePauseDialog() const
{
	vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialogLayer->GetControls();
	for(vector<DialogBox*>::iterator it = dialogs.begin(), end = dialogs.end(); it != end; ++it)
	{
		if((*it)->pause)
			return true;
	}
	return false;
}

//=================================================================================================
DialogBox* Gui::GetDialog(cstring name)
{
	assert(name);
	if(dialogLayer->Empty())
		return nullptr;
	vector<DialogBox*>& dialogs = (vector<DialogBox*>&)dialogLayer->GetControls();
	for(vector<DialogBox*>::iterator it = dialogs.begin(), end = dialogs.end(); it != end; ++it)
	{
		if((*it)->name == name)
			return *it;
	}
	return nullptr;
}

//=================================================================================================
void Gui::DrawSprite2(Texture* t, const Matrix& mat, const Rect* part, const Rect* clipping, Color color)
{
	assert(t && t->IsLoaded());

	Int2 size = t->GetSize();
	GuiRect rect;

	// set pos & uv
	if(part)
		rect.Set(size.x, size.y, *part);
	else
		rect.Set(size.x, size.y);

	// transform
	rect.Transform(mat);

	// clipping
	if(clipping && !rect.Clip(*clipping))
		return;

	// fill vertex buffer
	VGui* v = vBuf;
	rect.Populate(v, color);
	shader->Draw(t->tex, vBuf, 1);
}

//=================================================================================================
// Rotation is generaly ignored and shouldn't be used here
Rect Gui::GetSpriteRect(Texture* t, const Matrix& mat, const Rect* part, const Rect* clipping)
{
	assert(t && t->IsLoaded());

	Int2 size = t->GetSize();
	GuiRect rect;

	// set pos & uv
	if(part)
		rect.Set(size.x, size.y, *part);
	else
		rect.Set(size.x, size.y);

	// transform
	rect.Transform(mat);

	// clipping
	if(clipping && !rect.Clip(*clipping))
		return Rect::Zero;

	return rect.ToRect();
}

//=================================================================================================
void Gui::DrawArea(Color color, const Int2& pos, const Int2& size, const Box2d* clipRect)
{
	GuiRect guiRect;
	guiRect.Set(pos, size);
	if(!clipRect || guiRect.Clip(*clipRect))
	{
		const Vec4 col = color;
		VGui* v = vBuf;
		guiRect.Populate(v, col);
		shader->Draw(nullptr, vBuf, 1);
	}
}

//=================================================================================================
void Gui::DrawArea(const Box2d& rect, const AreaLayout& areaLayout, const Box2d* clipRect, Color* tint)
{
	if(areaLayout.mode == AreaLayout::Mode::None)
		return;

	Color color = areaLayout.color;
	if(tint)
		color *= *tint;

	if(areaLayout.mode == AreaLayout::Mode::Item)
	{
		DrawItem(areaLayout.tex, Int2(rect.LeftTop()), Int2(rect.Size()), color, areaLayout.size.x, areaLayout.size.y, clipRect);
	}
	else
	{
		// background
		if(areaLayout.mode == AreaLayout::Mode::Image && areaLayout.backgroundColor != Color::None)
		{
			assert(!clipRect);
			VGui* v = vBuf;
			AddRect(v, rect.LeftTop(), rect.RightBottom(), Color(areaLayout.backgroundColor));
			shader->Draw(nullptr, vBuf, 1);
		}

		// image/color
		GuiRect guiRect;
		TEX tex;
		if(areaLayout.mode >= AreaLayout::Mode::Image)
		{
			tex = areaLayout.tex->tex;
			guiRect.Set(rect, &areaLayout.region);
		}
		else
		{
			tex = nullptr;
			guiRect.Set(rect, nullptr);
		}
		if(clipRect)
		{
			if(!guiRect.Clip(*clipRect))
				return;
		}

		VGui* v = vBuf;
		guiRect.Populate(v, color);
		shader->Draw(tex, vBuf, 1);

		if(areaLayout.mode != AreaLayout::Mode::BorderColor)
			return;

		// border
		assert(!clipRect);
		v = vBuf;
		Vec4 col = areaLayout.borderColor;
		float s = (float)areaLayout.width;
		AddRect(v, rect.LeftTop(), rect.RightTop() + Vec2(-s, s), col);
		AddRect(v, rect.LeftTop(), rect.LeftBottom() + Vec2(s, 0), col);
		AddRect(v, rect.RightTop() + Vec2(-s, 0), rect.RightBottom(), col);
		AddRect(v, rect.LeftBottom() + Vec2(0, -s), rect.RightBottom(), col);
		shader->Draw(nullptr, vBuf, 4);
	}
}

//=================================================================================================
void Gui::DrawRect(Color color, const Rect& rect, int width)
{
	assert(width > 0);
	const Vec4 col = color;
	VGui* v = vBuf;
	AddRect(v, Vec2(float(rect.p1.x), float(rect.p1.y)), Vec2(float(rect.p2.x), float(rect.p1.y + width)), col);
	AddRect(v, Vec2(float(rect.p1.x), float(rect.p2.y - width)), Vec2(float(rect.p2.x), float(rect.p2.y)), col);
	AddRect(v, Vec2(float(rect.p1.x), float(rect.p1.y + width)), Vec2(float(rect.p1.x + width), float(rect.p2.y - width)), col);
	AddRect(v, Vec2(float(rect.p2.x - width), float(rect.p1.y + width)), Vec2(float(rect.p2.x), float(rect.p2.y - width)), col);
	shader->Draw(nullptr, vBuf, 4);
}

//=================================================================================================
void Gui::AddRect(VGui*& v, const Vec2& left_top, const Vec2& right_bottom, const Vec4& color)
{
	v->pos = Vec2(left_top.x, left_top.y);
	v->tex = Vec2(0, 0);
	v->color = color;
	++v;

	v->pos = Vec2(right_bottom.x, left_top.y);
	v->tex = Vec2(1, 0);
	v->color = color;
	++v;

	v->pos = Vec2(right_bottom.x, right_bottom.y);
	v->tex = Vec2(1, 1);
	v->color = color;
	++v;

	v->pos = Vec2(right_bottom.x, right_bottom.y);
	v->tex = Vec2(1, 1);
	v->color = color;
	++v;

	v->pos = Vec2(left_top.x, right_bottom.y);
	v->tex = Vec2(0, 1);
	v->color = color;
	++v;

	v->pos = Vec2(left_top.x, left_top.y);
	v->tex = Vec2(0, 0);
	v->color = color;
	++v;
}

//=================================================================================================
void Gui::SetClipboard(cstring text)
{
	assert(text);

	if(OpenClipboard(app::engine->GetWindowHandle()))
	{
		EmptyClipboard();
		uint length = strlen(text) + 1;
		HANDLE mem = GlobalAlloc(GMEM_FIXED, length);
		char* str = (char*)GlobalLock(mem);
		memcpy(str, text, length);
		GlobalUnlock(mem);
		SetClipboardData(CF_TEXT, mem);
		CloseClipboard();
	}
}

//=================================================================================================
cstring Gui::GetClipboard()
{
	cstring result = nullptr;

	if(OpenClipboard(app::engine->GetWindowHandle()))
	{
		if(IsClipboardFormatAvailable(CF_TEXT) != FALSE)
		{
			HANDLE mem = GetClipboardData(CF_TEXT);
			cstring str = (cstring)GlobalLock(mem);
			result = Format("%s", str);
			GlobalUnlock(mem);
		}
		CloseClipboard();
	}

	return result;
}

//=================================================================================================
void Gui::UseGrayscale(bool grayscale)
{
	assert(grayscale != this->grayscale);
	this->grayscale = grayscale;
	shader->SetGrayscale(grayscale ? 1.f : 0.f);
}

//=================================================================================================
bool Gui::DrawText2(DrawTextOptions& options)
{
	const int width = options.rect.SizeX();
	bool bottom_clip = false;

	DrawLineContext ctx;
	ctx.font = options.font;
	ctx.text = options.str;
	ctx.v = vBuf;
	ctx.v2 = (IsSet(options.flags, DTF_OUTLINE) && options.font->texOutline) ? vBuf2 : nullptr;
	ctx.inBuffer = 0;
	ctx.inBuffer2 = 0;
	ctx.parseSpecial = IsSet(options.flags, DTF_PARSE_SPECIAL);
	ctx.scale = options.scale;
	ctx.defColor = options.color;
	ctx.currentColor = ctx.defColor;
	if(options.hitboxes)
	{
		ctx.hc = &tmpHitboxContext;
		ctx.hc->hitbox = options.hitboxes;
		ctx.hc->counter = (options.hitboxCounter ? *options.hitboxCounter : 0);
		ctx.hc->open = HitboxOpen::No;
	}
	else
		ctx.hc = nullptr;

	if(!IsSet(options.flags, DTF_VCENTER | DTF_BOTTOM))
	{
		int y = options.rect.Top();

		if(!options.lines)
		{
			// tekst pionowo po œrodku lub na dole
			uint line_begin, line_end, line_index = 0;
			int line_width;
			while(options.font->SplitLine(line_begin, line_end, line_width, line_index, options.str, options.strLength, options.flags, width))
			{
				// pocz¹tkowa pozycja x w tej linijce
				int x;
				if(IsSet(options.flags, DTF_CENTER))
					x = options.rect.Left() + (width - line_width) / 2;
				else if(IsSet(options.flags, DTF_RIGHT))
					x = options.rect.Right() - line_width;
				else
					x = options.rect.Left();

				int clip_result = (options.clipping ? Clip(x, y, line_width, options.font->height, options.clipping) : 0);
				Int2 scaled_pos(int(options.scale.x * (x - options.rect.Left())) + options.rect.Left(),
					int(options.scale.y * (y - options.rect.Top())) + options.rect.Top());

				// znaki w tej linijce
				if(clip_result == 0)
					DrawTextLine(ctx, line_begin, line_end, scaled_pos.x, scaled_pos.y, nullptr);
				else if(clip_result == 5)
					DrawTextLine(ctx, line_begin, line_end, scaled_pos.x, scaled_pos.y, options.clipping);
				else if(clip_result == 2)
				{
					// tekst jest pod widocznym regionem, przerwij rysowanie
					bottom_clip = true;
					break;
				}
				else
				{
					// pomiñ hitbox
					SkipLine(options.str, line_begin, line_end, ctx.hc);
				}

				// zmieñ y na kolejn¹ linijkê
				y += options.font->height;
			}
		}
		else
		{
			for(uint line_index = options.linesStart, lines_max = min(options.linesEnd, options.lines->size()); line_index < lines_max; ++line_index)
			{
				auto& line = options.lines->at(line_index);

				// pocz¹tkowa pozycja x w tej linijce
				int x;
				if(IsSet(options.flags, DTF_CENTER))
					x = options.rect.Left() + (width - line.width) / 2;
				else if(IsSet(options.flags, DTF_RIGHT))
					x = options.rect.Right() - line.width;
				else
					x = options.rect.Left();

				int clip_result = (options.clipping ? Clip(x, y, line.width, options.font->height, options.clipping) : 0);
				Int2 scaled_pos(int(options.scale.x * (x - options.rect.Left())) + options.rect.Left(),
					int(options.scale.y * (y - options.rect.Top())) + options.rect.Top());

				// znaki w tej linijce
				if(clip_result == 0)
					DrawTextLine(ctx, line.begin, line.end, scaled_pos.x, scaled_pos.y, nullptr);
				else if(clip_result == 5)
					DrawTextLine(ctx, line.begin, line.end, scaled_pos.x, scaled_pos.y, options.clipping);
				else if(clip_result == 2)
				{
					// tekst jest pod widocznym regionem, przerwij rysowanie
					bottom_clip = true;
					break;
				}
				else
				{
					// pomiñ hitbox
					SkipLine(options.str, line.begin, line.end, ctx.hc);
				}

				// zmieñ y na kolejn¹ linijkê
				y += options.font->height;
			}
		}
	}
	else
	{
		// tekst u góry
		if(!options.lines)
		{
			static vector<TextLine> lines_data;
			lines_data.clear();

			// oblicz wszystkie linijki
			uint line_begin, line_end, line_index = 0;
			int line_width;
			while(options.font->SplitLine(line_begin, line_end, line_width, line_index, options.str, options.strLength, options.flags, width))
				lines_data.push_back(TextLine(line_begin, line_end, line_width));

			options.lines = &lines_data;
		}

		// pocz¹tkowa pozycja y
		int y;
		if(IsSet(options.flags, DTF_BOTTOM))
			y = options.rect.Bottom() - options.lines->size() * options.font->height;
		else
			y = options.rect.Top() + (options.rect.SizeY() - int(options.lines->size()) * options.font->height) / 2;

		for(uint line_index = options.linesStart, lines_max = min(options.linesEnd, options.lines->size()); line_index < lines_max; ++line_index)
		{
			auto& line = options.lines->at(line_index);

			// pocz¹tkowa pozycja x w tej linijce
			int x;
			if(IsSet(options.flags, DTF_CENTER))
				x = options.rect.Left() + (width - line.width) / 2;
			else if(IsSet(options.flags, DTF_RIGHT))
				x = options.rect.Right() - line.width;
			else
				x = options.rect.Left();

			int clip_result = (options.clipping ? Clip(x, y, line.width, options.font->height, options.clipping) : 0);
			Int2 scaled_pos(int(options.scale.x * (x - options.rect.Left())) + options.rect.Left(),
				int(options.scale.y * (y - options.rect.Top())) + options.rect.Top());

			// znaki w tej linijce
			if(clip_result == 0)
				DrawTextLine(ctx, line.begin, line.end, scaled_pos.x, scaled_pos.y, nullptr);
			else if(clip_result == 5)
				DrawTextLine(ctx, line.begin, line.end, scaled_pos.x, scaled_pos.y, options.clipping);
			else if(clip_result == 2)
			{
				// tekst jest pod widocznym regionem, przerwij rysowanie
				bottom_clip = true;
				break;
			}
			else if(options.hitboxes)
			{
				// pomiñ hitbox
				SkipLine(options.str, line.begin, line.end, ctx.hc);
			}

			// zmieñ y na kolejn¹ linijkê
			y += options.font->height;
		}
	}

	if(ctx.inBuffer2 > 0)
		shader->Draw(ctx.font->texOutline, vBuf2, ctx.inBuffer2);
	if(ctx.inBuffer > 0)
		shader->Draw(ctx.font->tex, vBuf, ctx.inBuffer);

	if(options.hitboxCounter)
		*options.hitboxCounter = ctx.hc->counter;

	return !bottom_clip;
}

//=================================================================================================
void Gui::SetLayout(Layout* masterLayout)
{
	assert(masterLayout);
	this->masterLayout = masterLayout;
	if(!layout)
		layout = masterLayout->Get<layout::Gui>();
}

//=================================================================================================
void Gui::RegisterControl(Control* control)
{
	assert(control);
	registeredControls.push_back(control);
}

//=================================================================================================
Box2d* Gui::SetClipRect(Box2d* clipRect)
{
	Box2d* prevClipRect = this->clipRect;
	this->clipRect = clipRect;
	return prevClipRect;
}
