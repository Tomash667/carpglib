#include "Pch.h"
#include "QuadTree.h"

enum QuadPartType
{
	Q_LEFT_BOTTOM,
	Q_RIGHT_BOTTOM,
	Q_LEFT_TOP,
	Q_RIGHT_TOP
};

void QuadTree::Init(delegate<Node*()> get, const Box2d& box, int splits)
{
	assert(!root);
	this->get = get;
	root = get();
	Init(root, box, splits);
}

void QuadTree::Init(Node* node, const Box2d& box, int splits)
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

bool QuadTree::List(FrustumPlanes& frustum, delegate<void(Node*)> callback)
{
	tmpNodes.clear();
	tmpNodes.push_back(root);

	bool any = false;
	while(!tmpNodes.empty())
	{
		Node* node = tmpNodes.back();
		tmpNodes.pop_back();
		if(frustum.BoxToFrustum(node->box))
		{
			any = true;
			callback(node);
			if(!node->leaf)
			{
				for(int i = 0; i < 4; ++i)
					tmpNodes.push_back(node->childs[i]);
			}
		}
	}

	return any;
}

void QuadTree::ListLeafs(FrustumPlanes& frustum, delegate<void(Node*)> callback)
{
	tmpNodes.clear();
	tmpNodes.push_back(root);

	while(!tmpNodes.empty())
	{
		Node* node = tmpNodes.back();
		tmpNodes.pop_back();
		if(frustum.BoxToFrustum(node->box))
		{
			if(!node->leaf)
			{
				for(int i = 0; i < 4; ++i)
					tmpNodes.push_back(node->childs[i]);
			}
			else
				callback(node);
		}
	}
}

void QuadTree::Clear(delegate<void(Node*)> callback)
{
	if(!root)
		return;

	tmpNodes.clear();
	tmpNodes.push_back(root);

	while(!tmpNodes.empty())
	{
		Node* node = tmpNodes.back();
		tmpNodes.pop_back();
		callback(node);
		for(int i = 0; i < 4; ++i)
		{
			if(node->childs[i])
				tmpNodes.push_back(node->childs[i]);
		}
	}

	root = nullptr;
}

QuadTree::Node* QuadTree::GetNode(const Vec2& pos, float radius)
{
	Node* node = root;
	if(!node->box.IsFullyInside(pos, radius))
		return root;

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

void QuadTree::ForEach(delegate<void(Node*)> callback)
{
	tmpNodes.clear();
	tmpNodes.push_back(root);

	while(!tmpNodes.empty())
	{
		Node* node = tmpNodes.back();
		tmpNodes.pop_back();
		callback(node);
		if(!node->leaf)
		{
			for(int i = 0; i < 4; ++i)
				tmpNodes.push_back(node->childs[i]);
		}
	}
}
