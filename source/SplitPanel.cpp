#include "Pch.h"
#include "SplitPanel.h"

SplitPanel::SplitPanel() : minSize1(0), minSize2(0), panel1(nullptr), panel2(nullptr), allowMove(true), horizontal(true), splitterSize(3)
{
}

SplitPanel::~SplitPanel()
{
	delete panel1;
	delete panel2;
}

void SplitPanel::Draw()
{
	gui->DrawArea(Box2d::Create(globalPos, size), layout->background);
	gui->DrawArea(Box2d(splitGlobal), horizontal ? layout->horizontal : layout->vertical);

	panel1->Draw();
	panel2->Draw();
}

void SplitPanel::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		assert(parent);
		if(!panel1)
			panel1 = new Panel;
		panel1->parent = this;
		if(!panel2)
			panel2 = new Panel;
		panel2->parent = this;
		Update(e, true, true);
		panel1->Initialize();
		panel2->Initialize();
		break;
	case GuiEvent_Moved:
		Update(e, false, true);
		break;
	case GuiEvent_Resize:
		Update(e, true, false);
		break;
	case GuiEvent_Show:
	case GuiEvent_Hide:
		panel1->Event(e);
		panel2->Event(e);
		break;
	case GuiEvent_LostMouseFocus:
		break;
	}
}

void SplitPanel::Update(float dt)
{
}

void SplitPanel::Update(GuiEvent e, bool resize, bool move)
{
	if(resize)
	{
		const Int2& padding = layout->padding;
		Int2 sizeLeft = size;
		if(horizontal)
		{
			sizeLeft.x -= splitterSize;
			panel1->size = Int2(sizeLeft.x / 2 - padding.x * 2, sizeLeft.y - padding.y * 2);
			panel1->pos = padding;
			split = Rect::Create(Int2(panel1->size.x + padding.x * 2, 0), Int2(splitterSize, size.y));
			panel2->size = Int2(sizeLeft.x - panel1->size.x - padding.x * 2, sizeLeft.y - padding.y * 2);
			panel2->pos = Int2(split.p1.x + padding.x, padding.y);
		}
		else
		{
			sizeLeft.y -= splitterSize;
			panel1->size = Int2(sizeLeft.x - padding.x * 2, sizeLeft.y / 2 - padding.y * 2);
			panel1->pos = padding;
			split = Rect::Create(Int2(0, panel1->size.y + padding.y), Int2(size.x, splitterSize));
			panel2->size = Int2(sizeLeft.x - padding.x * 2, sizeLeft.y - panel1->size.y - padding.y * 2);
			panel2->pos = Int2(padding.x, split.p1.y + padding.y);
		}
	}

	if(move)
	{
		globalPos = pos + parent->globalPos;
		panel1->globalPos = panel1->pos + globalPos;
		panel2->globalPos = panel2->pos + globalPos;
		splitGlobal += globalPos;
	}

	if(e != GuiEvent_Initialize)
	{
		panel1->Event(e);
		panel2->Event(e);
	}
}

void SplitPanel::SetPanel1(Panel* panel)
{
	assert(panel);
	assert(!panel1);
	panel1 = panel;
}

void SplitPanel::SetPanel2(Panel* panel)
{
	assert(panel);
	assert(!panel2);
	panel2 = panel;
}

void SplitPanel::SetSplitterSize(uint splitterSize)
{
	assert(!initialized);
	this->splitterSize = splitterSize;
}
