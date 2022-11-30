// show box tooltip for elements under cursor
#include "Pch.h"
#include "TooltipController.h"

//-----------------------------------------------------------------------------
static const int INVALID_INDEX = -1;
static const float TIMER = 0.3f;

//=================================================================================================
void TooltipController::Init(Callback getText)
{
	assert(getText);
	this->getText = getText;
	Clear();
}

//=================================================================================================
void TooltipController::Clear()
{
	group = INVALID_INDEX;
	id = INVALID_INDEX;
	anything = false;
	state = State::NOT_VISIBLE;
	alpha = 0.f;
	timer = 0.f;
}

//=================================================================================================
void TooltipController::UpdateTooltip(float dt, int new_group, int new_id)
{
	if(new_group != INVALID_INDEX)
	{
		if(new_group != group || new_id != id)
		{
			state = State::COUNTING;
			group = new_group;
			id = new_id;
			timer = TIMER;
		}
		else
			timer -= dt;

		if(state == State::COUNTING)
		{
			if(timer <= 0.f)
			{
				state = State::VISIBLE;
				alpha = 0.f;
				timer = 1.f;
				FormatBox(false);
			}
		}
		else
		{
			alpha += dt * 5;
			if(alpha >= 1.f)
				alpha = 1.f;
		}

		if(state == State::VISIBLE)
		{
			pos = gui->cursorPos + Int2(24, 24);
			if(pos.x + size.x >= gui->wndSize.x)
				pos.x = gui->wndSize.x - size.x - 1;
			if(pos.y + size.y >= gui->wndSize.y)
				pos.y = gui->wndSize.y - size.y - 1;
			timer -= dt;
			if(timer <= 0.f)
			{
				timer = 1.f;
				FormatBox(true);
			}
		}
	}
	else
	{
		state = State::NOT_VISIBLE;
		group = INVALID_INDEX;
		id = INVALID_INDEX;
	}
}

//=================================================================================================
void TooltipController::Draw()
{
	if(state != State::VISIBLE || !anything)
		return;

	int a = int(alpha * 222);

	// box
	gui->DrawArea(Box2d::Create(pos, size), layout->box);

	// image
	if(img)
	{
		if(imgSize == Int2::Zero)
			gui->DrawSprite(img, pos + Int2(12, 12), Color::Alpha(a));
		else
		{
			const Rect rect = Rect::Create(pos + Int2(12, 12), imgSize);
			gui->DrawSpriteRect(img, rect, Color::Alpha(a));
		}
	}

	Rect r;

	// big text
	if(!bigText.empty())
	{
		r = rBigText;
		r += pos;
		gui->DrawText(layout->fontBig, bigText, DTF_PARSE_SPECIAL, Color(0, 0, 0, a), r);
	}

	// text
	if(!text.empty())
	{
		r = rText;
		r += pos;
		gui->DrawText(layout->font, text, DTF_PARSE_SPECIAL, Color(0, 0, 0, a), r);
	}

	// small text
	if(!smallText.empty())
	{
		r = rSmallText;
		r += pos;
		gui->DrawText(layout->fontSmall, smallText, DTF_PARSE_SPECIAL, Color(0, 0, 0, a), r);
	}
}

//=================================================================================================
void TooltipController::FormatBox(bool refresh)
{
	getText(this, group, id, refresh);

	if(!anything)
		return;

	int w = 0, h = 0;

	Int2 img_size;
	if(img)
	{
		if(imgSize == Int2::Zero)
			img_size = img->GetSize();
		else
			img_size = imgSize;
	}
	else
		img_size = Int2::Zero;

	// big text
	if(!bigText.empty())
	{
		Int2 text_size = layout->fontBig->CalculateSize(bigText, 400);
		w = text_size.x;
		h = text_size.y + 12;
		rBigText.Left() = 0;
		rBigText.Right() = w;
		rBigText.Top() = 12;
		rBigText.Bottom() = h + 12;
	}

	// text
	Int2 text_size(0, 0);
	if(!text.empty())
	{
		if(h)
			h += 5;
		else
			h = 12;
		text_size = layout->font->CalculateSize(text, 400);
		if(text_size.x > w)
			w = text_size.x;
		rText.Left() = 0;
		rText.Right() = w;
		rText.Top() = h;
		h += text_size.y;
		rText.Bottom() = h;
	}

	int shift = 12;

	// image
	if(img)
	{
		shift += img_size.x + 4;
		w += img_size.x + 4;
	}

	// small text
	if(!smallText.empty())
	{
		if(h)
			h += 5;
		Int2 text_size = layout->fontSmall->CalculateSize(smallText, 400);
		text_size.x += 12;
		if(text_size.x > w)
			w = text_size.x;
		rSmallText.Left() = 12;
		rSmallText.Right() = w;
		rSmallText.Top() = h;
		h += text_size.y;
		rSmallText.Bottom() = h;

		int img_bot = img_size.y + 24;
		if(rSmallText.Top() < img_bot)
		{
			int dif = rSmallText.SizeY();
			rSmallText.Top() = img_bot;
			rSmallText.Bottom() = img_bot + dif;
			h = rSmallText.Bottom();
		}
	}
	else if(img && h < img_size.y + 12)
		h = img_size.y + 12;

	w += 24;
	h += 12;

	rBigText += Int2(shift, 0);
	rText += Int2(shift, 0);

	size = Int2(w, h);
}
