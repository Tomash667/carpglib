#pragma once

#include "VertexDeclaration.h"

// helper class to calculate clipping
class GuiRect
{
private:
	float mLeft, mTop, mRight, mBottom, mU, mV, mU2, mV2;

public:
	void Set(uint width, uint height)
	{
		mLeft = 0;
		mTop = 0;
		mRight = (float)width;
		mBottom = (float)height;
		mU = 0;
		mV = 0;
		mU2 = 1;
		mV2 = 1;
	}

	void Set(uint width, uint height, const Rect& part)
	{
		mLeft = (float)part.Left();
		mTop = (float)part.Top();
		mRight = (float)part.Right();
		mBottom = (float)part.Bottom();
		mU = mLeft / width;
		mV = mTop / height;
		mU2 = mRight / width;
		mV2 = mBottom / height;
	}

	void Set(const Box2d& box, const Box2d* uv)
	{
		mLeft = box.v1.x;
		mTop = box.v1.y;
		mRight = box.v2.x;
		mBottom = box.v2.y;
		if(uv)
		{
			mU = uv->v1.x;
			mV = uv->v1.y;
			mU2 = uv->v2.x;
			mV2 = uv->v2.y;
		}
		else
		{
			mU = 0;
			mV = 0;
			mU2 = 1;
			mV2 = 1;
		}
	}

	void Set(const Int2& pos, const Int2& size)
	{
		mLeft = (float)pos.x;
		mRight = (float)(pos.x + size.x);
		mTop = (float)pos.y;
		mBottom = (float)(pos.y + size.y);
	}

	void Set(const Vec2& pos1, const Vec2& pos2, const Vec2& uv1, const Vec2& uv2)
	{
		mLeft = pos1.x;
		mRight = pos2.x;
		mTop = pos1.y;
		mBottom = pos2.y;
		mU = uv1.x;
		mV = uv1.y;
		mU2 = uv2.x;
		mV2 = uv2.y;
	}

	void Transform(const Matrix& mat)
	{
		Vec2 leftTop(mLeft, mTop);
		Vec2 rightBottom(mRight, mBottom);
		leftTop = Vec2::Transform(leftTop, mat);
		rightBottom = Vec2::Transform(rightBottom, mat);
		mLeft = leftTop.x;
		mTop = leftTop.y;
		mRight = rightBottom.x;
		mBottom = rightBottom.y;
	}

	bool Clip(const Rect& clipping)
	{
		Box2d box(clipping);
		return Clip(box);
	}

	bool Clip(const Box2d& clipping)
	{
		int result = RequireClip(clipping);
		if(result == -1)
			return false; // no intersection
		else if(result == 0)
			return true; // fully inside

		const float left = max(mLeft, clipping.Left());
		const float right = min(mRight, clipping.Right());
		const float top = max(mTop, clipping.Top());
		const float bottom = min(mBottom, clipping.Bottom());
		const Vec2 orig_size(mRight - mLeft, mBottom - mTop);
		const float u = Lerp(mU, mU2, (left - mLeft) / orig_size.x);
		mU2 = Lerp(mU, mU2, 1.f - (mRight - right) / orig_size.x);
		mU = u;
		const float v = Lerp(mV, mV2, (top - mTop) / orig_size.y);
		mV2 = Lerp(mV, mV2, 1.f - (mBottom - bottom) / orig_size.y);
		mV = v;
		mLeft = left;
		mTop = top;
		mRight = right;
		mBottom = bottom;
		return true;
	}

	int RequireClip(const Box2d& c)
	{
		if(mLeft >= c.Right() || c.Left() >= mRight || mTop >= c.Bottom() || c.Top() >= mBottom)
			return -1; // no intersection
		if(mLeft >= c.Left() && mRight <= c.Right() && mTop >= c.Top() && mBottom <= c.Bottom())
			return 0; // fully inside
		return 1; // require clip
	}

	void Populate(VGui*& v, const Vec4& col)
	{
		v->pos = Vec2(mLeft, mTop);
		v->color = col;
		v->tex = Vec2(mU, mV);
		++v;

		v->pos = Vec2(mRight, mTop);
		v->color = col;
		v->tex = Vec2(mU2, mV);
		++v;

		v->pos = Vec2(mLeft, mBottom);
		v->color = col;
		v->tex = Vec2(mU, mV2);
		++v;

		v->pos = Vec2(mLeft, mBottom);
		v->color = col;
		v->tex = Vec2(mU, mV2);
		++v;

		v->pos = Vec2(mRight, mTop);
		v->color = col;
		v->tex = Vec2(mU2, mV);
		++v;

		v->pos = Vec2(mRight, mBottom);
		v->color = col;
		v->tex = Vec2(mU2, mV2);
		++v;
	}

	Rect ToRect() const
	{
		return Rect((int)mLeft, (int)mTop, (int)mRight, (int)mBottom);
	}
};
