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
void TooltipController::UpdateTooltip(float dt, int newGroup, int newId)
{
	if(newGroup != INVALID_INDEX)
	{
		if(newGroup != group || newId != id)
		{
			state = State::COUNTING;
			group = newGroup;
			id = newId;
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

	const int a = int(alpha * 222);
	const Color boxColor = Color::Alpha(a);
	const Color textColor = Color(0, 0, 0, a);

	// box
	gui->DrawArea(Box2d::Create(pos, size), layout->box, nullptr, &boxColor);

	// image
	if(img)
	{
		if(imgSize == Int2::Zero)
			gui->DrawSprite(img, pos + Int2(12, 12), boxColor);
		else
		{
			const Rect rect = Rect::Create(pos + Int2(12, 12), imgSize);
			gui->DrawSpriteRect(img, rect, boxColor);
		}
	}

	// big text
	if(!bigText.empty())
	{
		Rect r = rBigText;
		r += pos;
		gui->DrawText(layout->fontBig, bigText, DTF_PARSE_SPECIAL, textColor, r);
	}

	// text
	if(!text.empty())
	{
		Rect r = rText;
		r += pos;
		gui->DrawText(layout->font, text, DTF_PARSE_SPECIAL, textColor, r);
	}

	// small text
	if(!smallText.empty())
	{
		Rect r = rSmallText;
		r += pos;
		gui->DrawText(layout->fontSmall, smallText, DTF_PARSE_SPECIAL, textColor, r);
	}
}

//=================================================================================================
void TooltipController::FormatBox(bool refresh)
{
	getText(this, group, id, refresh);

	if(!anything)
		return;

	int w = 0, h = 0;

	Int2 tmpImgSize;
	if(img)
	{
		if(imgSize == Int2::Zero)
			tmpImgSize = img->GetSize();
		else
			tmpImgSize = imgSize;
	}
	else
		tmpImgSize = Int2::Zero;

	// big text
	if(!bigText.empty())
	{
		Int2 textSize = layout->fontBig->CalculateSize(bigText, 400);
		w = textSize.x;
		h = textSize.y + 12;
		rBigText.Left() = 0;
		rBigText.Right() = w;
		rBigText.Top() = 12;
		rBigText.Bottom() = h + 12;
	}

	// text
	Int2 textSize(0, 0);
	if(!text.empty())
	{
		if(h)
			h += 5;
		else
			h = 12;
		textSize = layout->font->CalculateSize(text, 400);
		if(textSize.x > w)
			w = textSize.x;
		rText.Left() = 0;
		rText.Right() = w;
		rText.Top() = h;
		h += textSize.y;
		rText.Bottom() = h;
	}

	int shift = 12;

	// image
	if(img)
	{
		shift += tmpImgSize.x + 4;
		w += tmpImgSize.x + 4;
	}

	// small text
	if(!smallText.empty())
	{
		if(h)
			h += 5;
		Int2 textSize = layout->fontSmall->CalculateSize(smallText, 400);
		textSize.x += 12;
		if(textSize.x > w)
			w = textSize.x;
		rSmallText.Left() = 12;
		rSmallText.Right() = w;
		rSmallText.Top() = h;
		h += textSize.y;
		rSmallText.Bottom() = h;

		int imgBottom = tmpImgSize.y + 24;
		if(rSmallText.Top() < imgBottom)
		{
			int dif = rSmallText.SizeY();
			rSmallText.Top() = imgBottom;
			rSmallText.Bottom() = imgBottom + dif;
			h = rSmallText.Bottom();
		}
	}
	else if(img && h < tmpImgSize.y + 12)
		h = tmpImgSize.y + 12;

	w += 24;
	h += 12;

	rBigText += Int2(shift, 0);
	rText += Int2(shift, 0);

	size = Int2(w, h);
}
