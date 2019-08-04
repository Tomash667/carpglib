#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
struct AreaLayout
{
	friend class LayoutLoader;

	enum class Mode
	{
		None,
		Color,
		BorderColor,
		Image,
		Item
	};

	Mode mode;
	Color color;
	union
	{
		struct
		{
			Color border_color;
			int width;
		};
		struct
		{
			Texture* tex;
			Box2d region;
			Color background_color;
		};
	};
	Int2 size;

	AreaLayout() : mode(Mode::None) {}
	explicit AreaLayout(Color color) : mode(Mode::Color), color(color) {}
	AreaLayout(Color color, Color border_color, int width = 1) : mode(Mode::BorderColor), color(color), border_color(border_color), width(width) {}
	explicit AreaLayout(Texture* tex, Color color = Color::White) : mode(Mode::Image), tex(tex), color(color), background_color(Color::None), region(0, 0, 1, 1)
	{
		SetFromArea(nullptr);
	}
	AreaLayout(Texture* tex, const Box2d& region) : mode(Mode::Image), tex(tex), color(Color::White), background_color(Color::White), region(region) {}
	AreaLayout(Texture* tex, const Rect& area) : mode(Mode::Image), tex(tex), color(Color::White), background_color(Color::None)
	{
		SetFromArea(&area);
	}
	AreaLayout(Texture* tex, int corner, int size, Color color = Color::White) : mode(Mode::Item), tex(tex), size(corner, size), color(color) {}
	AreaLayout(const AreaLayout& l) : mode(l.mode), color(l.color), size(l.size)
	{
		memcpy(&tex, &l.tex, sizeof(Texture*) + sizeof(Box2d) + sizeof(Color));
	}

	AreaLayout& operator = (const AreaLayout& l)
	{
		mode = l.mode;
		color = l.color;
		size = l.size;
		memcpy(&tex, &l.tex, sizeof(Texture*) + sizeof(Box2d) + sizeof(Color));
		return *this;
	}

	Box2d CalculateRegion(const Int2& pos, const Int2& region);

private:
	void SetFromArea(const Rect* area);
};

//-----------------------------------------------------------------------------
namespace layout
{
	struct Control
	{
	};

	struct Gui : public Control
	{
		TexturePtr cursor[CURSOR_MAX];
	};
}

//-----------------------------------------------------------------------------
class Layout
{
	friend class LayoutLoader;
public:
	Layout(Gui* gui);
	~Layout();
	layout::Control* Get(const type_info& type);
	template<typename T>
	T* Get()
	{
		return static_cast<T*>(Get(typeid(T)));
	}

private:
	Gui* gui;
	std::unordered_map<std::type_index, layout::Control*> types;
};

//-----------------------------------------------------------------------------
template<typename T>
class LayoutControl
{
	friend class Gui;

public:
	LayoutControl() : layout(Control::gui->GetLayout()->Get<T>()) {}

protected:
	T* layout;
};
