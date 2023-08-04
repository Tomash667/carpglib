#include "Pch.h"
#include "TreeView.h"

#include "Input.h"
#include "MenuStrip.h"
#include "TextBox.h"

/* TODO:
spacja - zaznacz
litera - przejdŸ do nastêpnego o tej literze

klik - zaznacza, z ctrl dodaje/usuwa zaznaczenie
z shift - zaznacza wszystkie od poprzedniego focusa do teraz
z shift zaznacza obszar od 1 klika do X, 1 miejsce sie nie zmienia
z shift i ctrl nie usuwa nigdy zaznaczenia (mo¿na dodaæ obszary)
*/

static bool SortTreeNodesPred(const TreeNode* node1, const TreeNode* node2)
{
	if(node1->IsDir() == node2->IsDir())
	{
		int r = _stricmp(node1->GetText().c_str(), node2->GetText().c_str());
		if(r == 0)
			return node1->GetText() < node2->GetText();
		else
			return r < 0;
	}
	else
		return node1->IsDir();
}

TreeNode::Enumerator::Iterator::Iterator(TreeNode* node, TreeNode::Pred pred) : node(node), pred(pred)
{
	if(node)
	{
		toCheck.push(node);
		Next();
	}
}

void TreeNode::Enumerator::Iterator::Next()
{
	while(true)
	{
		if(toCheck.empty())
		{
			node = nullptr;
			return;
		}

		TreeNode* n = toCheck.front();
		toCheck.pop();

		PredResult result;
		if(pred)
			result = pred(n);
		else
			result = GET_AND_CHECK_CHILDS;

		if(result == SKIP_AND_CHECK_CHILDS || result == GET_AND_CHECK_CHILDS)
		{
			for(auto child : n->childs)
				toCheck.push(child);
		}

		if(result == GET_AND_SKIP_CHILDS || result == GET_AND_CHECK_CHILDS)
		{
			node = n;
			break;
		}
	}
}

TreeNode::TreeNode(bool isDir) : tree(nullptr), parent(nullptr), selected(false), isDir(isDir), data(nullptr), collapsed(true)
{
}

TreeNode::~TreeNode()
{
	DeleteElements(childs);
}

void TreeNode::AddChild(TreeNode* node, bool expand)
{
	// dir przed item, sortowanie
	assert(node);
	assert(!node->tree);
	assert(tree);
	node->parent = this;
	node->tree = tree;
	node->CalculateWidth();
	node->CalculatePath(false);
	auto it = std::lower_bound(childs.begin(), childs.end(), node, SortTreeNodesPred);
	if(it == childs.end())
		childs.push_back(node);
	else
		childs.insert(it, node);
	if(expand || !collapsed)
	{
		collapsed = false;
		tree->CalculatePos();
	}
}

TreeNode* TreeNode::AddDirIfNotExists(const string& name, bool expand)
{
	TreeNode* dir = FindDir(name);
	if(!dir)
	{
		dir = new TreeNode(true);
		dir->SetText(name);
		AddChild(dir, expand);
	}
	return dir;
}

void TreeNode::DeattachChild(TreeNode* node)
{
	assert(node);
	assert(node->parent == this);
	tree->RemoveSelection(node);
	node->tree = nullptr;
	node->parent = nullptr;
	RemoveElementOrder(childs, node);
}

void TreeNode::EditName()
{
	tree->EditName(this);
}

TreeNode* TreeNode::FindDir(const string& name)
{
	for(auto node : childs)
	{
		if(node->isDir && node->text == name)
			return node;
	}

	return nullptr;
}

TreeNode::Enumerator TreeNode::ForEachNotDir()
{
	return Enumerator(this, [](TreeNode* node)
	{
		return node->IsDir() ? SKIP_AND_CHECK_CHILDS : GET_AND_CHECK_CHILDS;
	});
}

TreeNode::Enumerator TreeNode::ForEachVisible()
{
	return Enumerator(this, [](TreeNode* node)
	{
		if(node->IsDir() && node->IsCollapsed())
			return GET_AND_SKIP_CHILDS;
		else
			return GET_AND_CHECK_CHILDS;
	});
}

void TreeNode::GenerateDirName(TreeNode* node, cstring name)
{
	assert(node && name);
	node->text = name;
	uint index = 1;
	while(true)
	{
		bool ok = true;
		for(auto child : childs)
		{
			if(child->text == node->text)
			{
				ok = false;
				break;
			}
		}
		if(ok)
			return;
		node->text = Format("%s (%u)", name, index);
		++index;
	}
}

void TreeNode::Remove()
{
	tree->Remove(this);
}

void TreeNode::Select()
{
	tree->SelectNode(this);
}

void TreeNode::SetCollapsed(bool collapsed)
{
	if(this->collapsed == collapsed)
		return;
	this->collapsed = collapsed;
	if(!childs.empty())
		tree->CalculatePos();
}

void TreeNode::SetText(Cstring s)
{
	if(tree && parent)
	{
		auto it = std::lower_bound(parent->childs.begin(), parent->childs.end(), this, SortTreeNodesPred);
		parent->childs.erase(it);
		text = s.s;
		it = std::lower_bound(parent->childs.begin(), parent->childs.end(), this, SortTreeNodesPred);
		if(it == parent->childs.end())
			parent->childs.push_back(this);
		else
			parent->childs.insert(it, this);
		CalculateWidth();
		if(IsDir())
			CalculatePath(true);
		tree->CalculatePos();
	}
	else
		text = s.s;
}

void TreeNode::CalculateWidth()
{
	width = tree->layout->font->CalculateSize(text).x + 2;
	if(IsDir())
		width += tree->layout->button.size.x;
}

void TreeNode::CalculatePath(bool sendEvent)
{
	cstring parentPath = (parent->IsRoot() ? nullptr : parent->path.c_str());
	cstring newPath;
	if(IsDir())
	{
		if(parentPath)
			newPath = Format("%s/%s", parentPath, text.c_str());
		else
			newPath = text.c_str();
	}
	else
	{
		if(parentPath)
			newPath = parentPath;
		else
			newPath = "";
	}

	if(path != newPath)
	{
		path = newPath;
		if(sendEvent)
			tree->handler(TreeView::A_PATH_CHANGED, (int)this);
		if(IsDir())
		{
			for(auto child : childs)
				child->CalculatePath(sendEvent);
		}
	}
}

TreeView::TreeView() : Control(true), TreeNode(true), menu(nullptr), hover(nullptr), edited(nullptr), fixed(nullptr), drag(DRAG_NO), hscrollbar(true, true),
vscrollbar(false, true)
{
	tree = this;
	text = "Root";
	collapsed = false;
	textBox = new TextBox(true);
	textBox->visible = false;
	hscrollbar.visible = false;
	vscrollbar.visible = false;
	CalculateWidth();
	SetOnCharHandler(true);
}

TreeView::~TreeView()
{
	delete textBox;
}

void TreeView::CalculatePos()
{
	Int2 offset(0, 0);
	int maxWidth = 0;
	CalculatePos(this, offset, maxWidth);
	totalSize = Int2(maxWidth, offset.y);
	areaSize = size - Int2(2, 2);

	bool useHscrollbar = false, useVscrollbar = false;
	if(totalSize.y > areaSize.y)
	{
		useVscrollbar = true;
		areaSize.x -= 15;
	}
	if(totalSize.x > areaSize.x)
	{
		useHscrollbar = true;
		areaSize.y -= 15;
		if(!useVscrollbar && totalSize.y > areaSize.y)
		{
			useVscrollbar = true;
			areaSize.x -= 15;
		}
	}

	int sizeSub = (useHscrollbar && useVscrollbar) ? 15 : 0;
	if(useHscrollbar)
	{
		hscrollbar.SetSize(Int2(size.x - sizeSub, 16));
		hscrollbar.part = size.x - sizeSub;
		hscrollbar.visible = true;
		hscrollbar.UpdateTotal(totalSize.x);
	}
	else
	{
		hscrollbar.visible = false;
		hscrollbar.offset = 0.f;
	}
	if(useVscrollbar)
	{
		vscrollbar.SetSize(Int2(16, size.y - sizeSub));
		vscrollbar.part = size.y - sizeSub;
		vscrollbar.visible = true;
		vscrollbar.UpdateTotal(totalSize.y);
	}
	else
	{
		vscrollbar.visible = false;
		vscrollbar.offset = 0.f;
	}
}

void TreeView::CalculatePos(TreeNode* node, Int2& offset, int& maxWidth)
{
	node->pos = offset;
	int width = node->pos.x + node->width;
	maxWidth = max(width, maxWidth);
	offset.y += itemHeight;
	if(node->IsDir() && !node->IsCollapsed())
	{
		offset.x += levelOffset;
		for(auto child : node->childs)
			CalculatePos(child, offset, maxWidth);
		offset.x -= levelOffset;
		node->endOffset = offset.y;
	}
}

void TreeView::Draw()
{
	Box2d box = Box2d::Create(globalPos, size);
	gui->DrawArea(box, layout->background);

	if(hscrollbar.visible)
		hscrollbar.Draw();
	if(vscrollbar.visible)
		vscrollbar.Draw();

	clipRect = Box2d::Create(globalPos + Int2(1, 1), areaSize);
	Draw(this);

	if(textBox->visible)
	{
		Box2d* prevClipRect = gui->SetClipRect(&clipRect);
		textBox->Draw();
		gui->SetClipRect(prevClipRect);
	}

	if(drag == DRAG_MOVED)
		gui->DrawSprite(layout->dragAndDrop, gui->cursorPos + Int2(16, 16));
}

void TreeView::Draw(TreeNode* node)
{
	int offsetx = -(int)hscrollbar.offset;
	int offsety = node->pos.y - (int)vscrollbar.offset;
	if(offsety > size.y)
		return; // below view
	if(offsety + itemHeight >= 0)
	{
		// selection
		if(node->selected || (drag == DRAG_MOVED && node == above && CanDragAndDrop()))
			gui->DrawArea(Box2d::Create(globalPos + Int2(1, 1 + offsety), Int2(areaSize.x, itemHeight)), layout->selected, &clipRect);

		if(node->IsDir())
		{
			// collapse/expand button
			AreaLayout* area;
			if(node->collapsed)
			{
				if(node == hover)
					area = &layout->buttonHover;
				else
					area = &layout->button;
			}
			else
			{
				if(node == hover)
					area = &layout->buttonDownHover;
				else
					area = &layout->buttonDown;
			}
			gui->DrawArea(Box2d::Create(globalPos + Int2(node->pos.x + offsetx, offsety), area->size), *area, &clipRect);
			offsetx += area->size.x;
		}

		// text
		if(node != edited)
		{
			const Rect r = {
				globalPos.x + node->pos.x + offsetx,
				globalPos.y + offsety,
				globalPos.x + size.x,
				globalPos.y + itemHeight + offsety
			};
			const Rect clipRect(clipRect);
			gui->DrawText(layout->font, node->text, DTF_LEFT | DTF_VCENTER | DTF_SINGLELINE, layout->fontColor, r, &clipRect);
		}
	}

	// childs
	if(node->IsDir() && !node->IsCollapsed())
	{
		int endOffset = (int)vscrollbar.offset + node->endOffset;
		if(endOffset >= 0)
		{
			for(auto child : node->childs)
				Draw(child);
		}
	}
}

void TreeView::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		itemHeight = layout->font->height + 2;
		levelOffset = layout->levelOffset;
		vscrollbar.size = Int2(16, size.y);
		vscrollbar.part = size.y;
		vscrollbar.pos = Int2(size.x - 16, 0);
		vscrollbar.globalPos = globalPos + vscrollbar.pos;
		hscrollbar.size = Int2(size.x, 16);
		hscrollbar.part = size.x;
		hscrollbar.pos = Int2(0, size.y - 16);
		hscrollbar.globalPos = globalPos + hscrollbar.pos;
		CalculatePos();
		break;
	case GuiEvent_LostFocus:
		drag = DRAG_NO;
		break;
	case GuiEvent_Moved:
		vscrollbar.globalPos = globalPos + vscrollbar.pos;
		hscrollbar.globalPos = globalPos + hscrollbar.pos;
		break;
	case GuiEvent_Resize:
		break;
	}
}

void TreeView::OnChar(char c)
{
	if(c >= 'A' && c <= 'Z')
		c = tolower(c);

	int startY;
	if(current)
		startY = current->pos.y + 1;
	else
		startY = -1;

	TreeNode* firstAbove = nullptr;
	TreeNode* firstBelow = nullptr;
	for(auto node : ForEachVisible())
	{
		if(node->pos.y < startY)
		{
			if(!firstAbove)
			{
				char startsWith = tolower(node->text[0]);
				if(c == startsWith)
					firstAbove = node;
			}
			continue;
		}

		char startsWith = tolower(node->text[0]);
		if(c == startsWith)
		{
			firstBelow = node;
			break;
		}
	}

	TreeNode* node = (firstBelow ? firstBelow : firstAbove);
	if(node && node != current)
	{
		if(SelectNode(node, false, false, false))
			ScrollTo(node);
	}
}

void TreeView::Update(float dt)
{
	hover = nullptr;

	// update edit box
	if(textBox->visible)
	{
		SetTextboxLocation();
		UpdateControl(textBox, dt);
		if(!textBox->focus)
			EndEdit(true, false);
		else
		{
			if(input->PressedRelease(Key::Enter))
				EndEdit(true, true);
			else if(input->PressedRelease(Key::Escape))
				EndEdit(false, true);
		}
	}

	// scrollbars
	bool oldMouseFocus = mouseFocus;
	if(hscrollbar.visible)
	{
		UpdateControl(&hscrollbar, dt);
		if(oldMouseFocus && input->Down(Key::Shift))
			hscrollbar.ApplyMouseWheel();
	}
	if(vscrollbar.visible)
	{
		UpdateControl(&vscrollbar, dt);
		if(oldMouseFocus && !input->Down(Key::Shift))
			vscrollbar.ApplyMouseWheel();
	}

	// recursively update nodes
	above = nullptr;
	if(mouseFocus)
		Update(this);

	// drag & drop
	if(drag != DRAG_NO)
	{
		// scroll when draging
		if(vscrollbar.visible)
		{
			const float DRAG_SCROLL_SPEED_MIN = 50.f;
			const float DRAG_SCROLL_SPEED_MAX = 400.f;
			int posy = gui->cursorPos.y - globalPos.y;
			if(posy >= 0 && posy <= itemHeight * 2)
			{
				float speed = Lerp(DRAG_SCROLL_SPEED_MIN, DRAG_SCROLL_SPEED_MAX, ((float)(itemHeight * 2) - posy) / (itemHeight * 2));
				vscrollbar.UpdateOffset(-speed * dt);
			}
			else if(posy >= size.y - itemHeight * 2 && posy <= size.y)
			{
				float speed = Lerp(DRAG_SCROLL_SPEED_MIN, DRAG_SCROLL_SPEED_MAX, ((float)posy - size.y + itemHeight * 2) / (itemHeight * 2));
				vscrollbar.UpdateOffset(+speed * dt);
			}
		}

		if(drag == DRAG_DOWN && dragNode != above && above)
			drag = DRAG_MOVED;
		if(input->Up(Key::LeftButton))
		{
			if(above == dragNode)
				SelectNode(dragNode, input->Down(Key::Shift), false, input->Down(Key::Control));
			else if(CanDragAndDrop())
			{
				auto oldSelected = selectedNodes;
				SelectTopSelectedNodes();
				above->collapsed = false;
				for(auto node : selectedNodes)
				{
					MoveNode(node, above);
					node->selected = false;
				}
				selectedNodes = oldSelected;
				for(auto node : selectedNodes)
					node->selected = true;
				CalculatePos();
			}
			drag = DRAG_NO;
		}
		else if(input->PressedRelease(Key::Escape))
			drag = DRAG_NO;
	}

	// keyboard shortcuts
	if(focus && current && drag == DRAG_NO)
	{
		if(input->DownRepeat(Key::Up))
			MoveCurrent(-1, input->Down(Key::Shift));
		else if(input->DownRepeat(Key::Down))
			MoveCurrent(+1, input->Down(Key::Shift));
		else if(input->PressedRelease(Key::Left))
		{
			if(current->IsDir())
				current->SetCollapsed(true);
		}
		else if(input->PressedRelease(Key::Right))
		{
			if(current->IsDir())
				current->SetCollapsed(false);
		}
		else if(input->Shortcut(KEY_CONTROL, Key::R))
			handler(A_SHORTCUT, S_RENAME);
		else if(input->PressedRelease(Key::Delete))
			handler(A_SHORTCUT, S_REMOVE);
		else if(input->Shortcut(KEY_CONTROL, Key::A))
			handler(A_SHORTCUT, S_ADD);
		else if(input->Shortcut(KEY_CONTROL | KEY_SHIFT, Key::A))
			handler(A_SHORTCUT, S_ADD_DIR);
		else if(input->Shortcut(KEY_CONTROL, Key::D))
			handler(A_SHORTCUT, S_DUPLICATE);
	}
}

void TreeView::EndEdit(bool apply, bool setFocus)
{
	if(setFocus)
		SetFocus();
	textBox->visible = false;
	if(!apply)
	{
		edited = nullptr;
		return;
	}
	newName = textBox->GetText();
	Trim(newName);
	if(newName != edited->text)
	{
		if(!newName.empty())
		{
			bool ok = true;
			for(auto child : edited->parent->childs)
			{
				if(child != edited && child->IsDir() == edited->IsDir() && child->text == newName)
				{
					ok = false;
					break;
				}
			}

			if(!ok)
			{
				SimpleDialog("Name must be unique.");
				SelectNode(edited);
			}
			else if(handler(A_BEFORE_RENAME, (int)edited))
			{
				edited->SetText(newName);
				handler(A_RENAMED, (int)edited);
			}
		}
		else
		{
			SimpleDialog("Name cannot be empty.");
			SelectNode(edited);
		}
	}
	edited = nullptr;
}

bool TreeView::Update(TreeNode* node)
{
	int offsety = node->pos.y - (int)vscrollbar.offset;
	if(offsety > size.y)
		return false; // below view

	if(offsety + itemHeight >= 0)
	{
		if(gui->cursorPos.y >= globalPos.y + offsety && gui->cursorPos.y <= globalPos.y + offsety + itemHeight)
		{
			above = node;

			bool add = input->Down(Key::Shift);
			bool ctrl = input->Down(Key::Control);

			if(menu && input->Pressed(Key::RightButton))
			{
				if(SelectNode(node, add, true, ctrl) && handler(A_BEFORE_MENU_SHOW, (int)node))
				{
					menu->SetHandler(delegate<void(int)>(this, &TreeView::OnSelect));
					menu->ShowMenu();
				}
			}

			int offsetx = -(int)hscrollbar.offset;
			if(node->IsDir() && Rect::IsInside(gui->cursorPos, globalPos.x + node->pos.x + offsetx, globalPos.y + offsety,
				globalPos.x + node->pos.x + 16 + offsetx, globalPos.y + offsety + itemHeight))
			{
				hover = node;
				if(input->Pressed(Key::LeftButton))
				{
					node->SetCollapsed(!node->IsCollapsed());
					SelectNode(node, add, false, ctrl);
					TakeFocus(true);
				}
				return true;
			}

			if(input->Pressed(Key::LeftButton))
			{
				if(!node->selected)
					SelectNode(node, add, false, ctrl);
				TakeFocus(true);
				drag = DRAG_DOWN;
				dragNode = node;
			}
			return true;
		}
	}

	if(node->IsDir() && !node->IsCollapsed())
	{
		int endOffset = (int)vscrollbar.offset + node->endOffset;
		if(endOffset >= 0)
		{
			for(auto child : node->childs)
			{
				if(Update(child))
					return true;
			}
		}
	}

	return false;
}

void TreeView::Add(TreeNode* node, const string& path, bool expand)
{
	assert(node);

	if(path.empty())
	{
		AddChild(node, expand);
		return;
	}

	uint pos = 0;
	TreeNode* container = this;
	string part;

	while(true)
	{
		uint end = path.find_first_of('/', pos);
		if(end == string::npos)
			break;
		part = path.substr(pos, end - pos);
		container = container->AddDirIfNotExists(part, expand);
		pos = end + 1;
	}

	part = path.substr(pos);
	container = container->AddDirIfNotExists(part, expand);
	container->AddChild(node, expand);
}

void TreeView::ClearSelection()
{
	for(auto node : selectedNodes)
		node->selected = false;
	selectedNodes.clear();
	current = nullptr;
}

void TreeView::Deattach(TreeNode* node)
{
	assert(node);
	assert(node->tree == this && node->parent != nullptr); // root node have null parent
	RemoveSelection(node);
	RemoveElementOrder(node->parent->childs, node);
	node->parent = nullptr;
	node->tree = nullptr;
}

void TreeView::EditName(TreeNode* node)
{
	assert(node && node->tree == this);
	if(!SelectNode(node))
		return;
	edited = node;
	textBox->visible = true;
	textBox->SetText(node->text.c_str());
	SetTextboxLocation();
	textBox->SelectAll();
}

bool TreeView::SelectNode(TreeNode* node, bool add, bool rightClick, bool ctrl)
{
	assert(node && node->tree == this);

	if(current == node && (add || rightClick))
		return true;

	if(!handler(A_BEFORE_CURRENT_CHANGE, (int)node))
		return false;

	if(rightClick && node->selected)
	{
		current = node;
		handler(A_CURRENT_CHANGED, (int)node);
		return true;
	}

	// deselect old
	if(!ctrl)
	{
		for(auto node : selectedNodes)
			node->selected = false;
		selectedNodes.clear();
	}

	if(add && fixed)
		SelectRange(fixed, node);
	else
	{
		// select new
		if(!node->selected)
		{
			node->selected = true;
			selectedNodes.push_back(node);
		}
		fixed = node;
	}

	current = node;
	handler(A_CURRENT_CHANGED, (int)node);

	return true;
}

void TreeView::Remove(TreeNode* node)
{
	assert(node);
	assert(node->tree == this && node->parent != nullptr); // root node have null parent
	RemoveSelection(node);
	TreeNode* next;
	auto parent = node->parent;
	int index = GetIndex(parent->childs, node);
	if(index + 1 < (int)parent->childs.size())
		next = parent->childs[index + 1];
	else if(index > 0)
		next = parent->childs[index - 1];
	else
		next = parent;
	RemoveElementOrder(node->parent->childs, node);
	delete node;
	CalculatePos();
	SelectNode(next);
}

void TreeView::RemoveSelection(TreeNode* node)
{
	if(node->selected)
	{
		RemoveElement(selectedNodes, node);
		node->selected = false;
	}
	if(current == node)
		current = nullptr;
	for(auto child : node->childs)
		RemoveSelection(child);
}

void TreeView::OnSelect(int id)
{
	handler(A_MENU, id);
}

TreeNode* TreeView::GetNextNode(int dir)
{
	if(dir == -1)
	{
		if(current->parent == nullptr)
			return nullptr;

		auto node = current;
		int index = GetIndex(node->parent->childs, node);
		if(index != 0)
		{
			node = node->parent->childs[index - 1];
			while(node->IsDir() && !node->IsCollapsed() && !node->childs.empty())
				node = node->childs.back();
		}
		else
			node = node->parent;
		return node;
	}
	else
	{
		if(current->parent == nullptr)
		{
			if(!current->childs.empty())
				return current->childs[0];
			else
				return nullptr;
		}

		auto node = current;
		if(node->IsDir() && !node->IsCollapsed() && !node->childs.empty())
			node = current->childs.front();
		else
		{
			while(true)
			{
				int index = GetIndex(node->parent->childs, node);
				if(index + 1 != node->parent->childs.size())
				{
					node = node->parent->childs[index + 1];
					break;
				}
				else
				{
					node = node->parent;
					if(node->parent == nullptr)
						return nullptr;
				}
			}
		}
		return node;
	}
}

void TreeView::MoveCurrent(int dir, bool add)
{
	TreeNode* next = GetNextNode(dir);
	if(next)
	{
		SelectNode(next, add, false, false);
		ScrollTo(next);
	}
}

void TreeView::SelectRange(TreeNode* node1, TreeNode* node2)
{
	if(node1->pos.y > node2->pos.y)
		std::swap(node1, node2);
	auto e = ForEach([](TreeNode* node)
	{
		if(node->IsDir() && node->IsCollapsed())
			return GET_AND_SKIP_CHILDS;
		else
			return GET_AND_CHECK_CHILDS;
	});
	for(auto node : e)
	{
		if(!node->selected && node->pos.y >= node1->pos.y && node->pos.y <= node2->pos.y)
		{
			node->selected = true;
			selectedNodes.push_back(node);
		}
	}
}

void TreeView::RemoveSelected()
{
	if(selectedNodes.empty())
		return;

	if(selectedNodes.size() == 1u)
	{
		Remove(current);
		return;
	}

	auto node = current;
	// get not selected parent
	while(node->parent->selected)
		node = node->parent;
	bool ok = !node->selected;
	if(!ok)
	{
		// get item below
		int index = GetIndex(node->parent->childs, node);
		int startIndex = index;
		while(true)
		{
			++index;
			if(index == node->parent->childs.size())
				break;
			if(!node->parent->childs[index]->selected)
			{
				ok = true;
				node = node->parent->childs[index];
				break;
			}
		}

		// get item above
		if(!ok)
		{
			index = startIndex;
			while(true)
			{
				--index;
				if(index < 0)
					break;
				if(!node->parent->childs[index]->selected)
				{
					ok = true;
					node = node->parent->childs[index];
					break;
				}
			}
		}

		// get parent
		if(!ok)
			node = node->parent;
	}

	// remove nodes
	SelectTopSelectedNodes();
	for(auto node : selectedNodes)
	{
		RemoveElementOrder(node->parent->childs, node);
		delete node;
	}
	selectedNodes.clear();

	// select new
	CalculatePos();
	SelectNode(node);
}

void TreeView::ScrollTo(TreeNode* node)
{
	assert(node);
	int offsety = node->pos.y - (int)vscrollbar.offset;
	if(offsety < 0)
		vscrollbar.offset = (float)node->pos.y;
	else if(offsety + 16 > size.y)
		vscrollbar.offset = (float)(itemHeight + node->pos.y - size.y);
}

void TreeView::SelectChildNodes()
{
	for(auto node : selectedNodes)
		SelectChildNodes(node);
}

void TreeView::SelectChildNodes(TreeNode* node)
{
	for(auto child : node->childs)
	{
		if(!child->selected)
		{
			child->selected = true;
			if(!child->childs.empty())
				SelectChildNodes(child);
		}
	}
}

void TreeView::SelectTopSelectedNodes()
{
	SelectChildNodes();
	LoopAndRemove(selectedNodes, [](TreeNode* node) { return node->GetParent()->IsSelected(); });
}

bool TreeView::CanDragAndDrop()
{
	return above && !above->selected && above->IsDir();
}

bool TreeView::MoveNode(TreeNode* node, TreeNode* newParent)
{
	if(node->parent == newParent)
		return false;
	RemoveElementOrder(node->parent->childs, node);
	node->parent = newParent;
	auto it = std::lower_bound(newParent->childs.begin(), newParent->childs.end(), node, SortTreeNodesPred);
	if(it == newParent->childs.end())
		newParent->childs.push_back(node);
	else
		newParent->childs.insert(it, node);
	node->CalculatePath(true);
	return true;
}

void TreeView::SetAllCollapsed(bool collapsed)
{
	for(auto node : ForEach())
	{
		if(node->IsDir())
			node->collapsed = collapsed;
	}
	CalculatePos();
}

void TreeView::SetTextboxLocation()
{
	int startpos = edited->pos.x;
	Int2 pos = globalPos + edited->pos;
	pos.x -= (int)hscrollbar.offset + 4;
	pos.y -= (int)vscrollbar.offset + 2;
	if(edited->IsDir())
	{
		pos.x += layout->button.size.x;
		startpos += layout->button.size.x;
	}
	textBox->SetPosition(pos);

	int width = layout->font->CalculateSize(textBox->GetText()).x + 10;
	if(startpos + width > areaSize.x)
		width = areaSize.x - startpos;
	Int2 newSize = Int2(width, itemHeight + 4);
	if(newSize != textBox->GetSize())
	{
		textBox->SetSize(newSize);
		textBox->CalculateOffset(false);
	}
}
