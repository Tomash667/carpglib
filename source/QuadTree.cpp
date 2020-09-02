#include "Pch.h"
#include "QuadTree.h"

enum QuadPartType
{
	Q_LEFT_BOTTOM,
	Q_RIGHT_BOTTOM,
	Q_LEFT_TOP,
	Q_RIGHT_TOP
};

void QuadTree::Init(QuadNode* node, const Box2d& box, int splits)
{
	if(node)
	{
		node->box = box;
		if(splits > 0)
		{
			node->leaf = false;
			--splits;
			node->childs[Q_LEFT_BOTTOM] = get();
			Init(node->childs[Q_LEFT_BOTTOM], box.LeftBottomPart(), splits);
			node->childs[Q_RIGHT_BOTTOM] = get();
			Init(node->childs[Q_RIGHT_BOTTOM], box.RightBottomPart(), splits);
			node->childs[Q_LEFT_TOP] = get();
			Init(node->childs[Q_LEFT_TOP], box.LeftTopPart(), splits);
			node->childs[Q_RIGHT_TOP] = get();
			Init(node->childs[Q_RIGHT_TOP], box.RightTopPart(), splits);
		}
		else
		{
			node->leaf = true;
			for(int i = 0; i < 4; ++i)
				node->childs[i] = nullptr;
		}
	}
	else
	{
		top = get();
		Init(top, box, splits);
	}
}

void QuadTree::List(FrustumPlanes& frustum, Nodes& nodes)
{
	nodes.clear();
	tmp_nodes.clear();
	tmp_nodes.push_back(top);

	while(!tmp_nodes.empty())
	{
		QuadNode* node = tmp_nodes.back();
		tmp_nodes.pop_back();
		if(frustum.BoxToFrustum(node->box))
		{
			nodes.push_back(node);
			if(!node->leaf)
			{
				for(int i = 0; i < 4; ++i)
					tmp_nodes.push_back(node->childs[i]);
			}
		}
	}
}

void QuadTree::ListLeafs(FrustumPlanes& frustum, Nodes& nodes)
{
	nodes.clear();
	tmp_nodes.clear();
	tmp_nodes.push_back(top);

	while(!tmp_nodes.empty())
	{
		QuadNode* node = tmp_nodes.back();
		tmp_nodes.pop_back();
		if(frustum.BoxToFrustum(node->box))
		{
			if(!node->leaf)
			{
				for(int i = 0; i < 4; ++i)
					tmp_nodes.push_back(node->childs[i]);
			}
			else
				nodes.push_back(node);
		}
	}
}

void QuadTree::Clear(Nodes& nodes)
{
	if(!top)
		return;

	nodes.clear();
	tmp_nodes.clear();
	tmp_nodes.push_back(top);

	while(!tmp_nodes.empty())
	{
		QuadNode* node = tmp_nodes.back();
		tmp_nodes.pop_back();
		nodes.push_back(node);
		for(int i = 0; i < 4; ++i)
		{
			if(node->childs[i])
				tmp_nodes.push_back(node->childs[i]);
		}
	}

	top = nullptr;
}

QuadNode* QuadTree::GetNode(const Vec2& pos, float radius)
{
	QuadNode* node = top;
	if(!node->box.IsFullyInside(pos, radius))
		return top;

	while(!node->leaf)
	{
		Vec2 midpoint = node->box.Midpoint();
		int index;
		if(pos.x >= midpoint.x)
		{
			if(pos.y >= midpoint.y)
				index = Q_RIGHT_BOTTOM;
			else
				index = Q_RIGHT_TOP;
		}
		else
		{
			if(pos.y >= midpoint.y)
				index = Q_LEFT_BOTTOM;
			else
				index = Q_LEFT_TOP;
		}

		if(node->childs[index]->box.IsFullyInside(pos, radius))
			node = node->childs[index];
		else
			break;
	}

	return node;
}
