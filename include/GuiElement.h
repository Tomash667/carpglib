#pragma once

//-----------------------------------------------------------------------------
class GuiElement
{
public:
	GuiElement(int value = 0, Texture* tex = nullptr) : value(value), tex(tex)
	{
	}
	virtual ~GuiElement() {}
	virtual cstring ToString() = 0;
	template<typename T>
	inline T* Cast() { return static_cast<T*>(this); }

	int value, height;
	Texture* tex;
};

//-----------------------------------------------------------------------------
class DefaultGuiElement : public GuiElement
{
public:
	DefaultGuiElement(cstring text, int value = 0, Texture* tex = nullptr) : GuiElement(value, tex)
	{
		this->text = text;
	}

	cstring ToString() override
	{
		return text.c_str();
	}

	string text;
};
