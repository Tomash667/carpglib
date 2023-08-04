#pragma once

struct QuadNode
{
	Box2d box;
	Rect gridBox;
	QuadNode* childs[4];
	bool leaf;
};

typedef QuadNode* (*GetQuadNode)();

struct QuadTree
{
	typedef vector<QuadNode*> Nodes;

	QuadTree() : top(nullptr)
	{
	}

	void Init(QuadNode* node, const Box2d& box, const Rect& gridBox, int splits);

	void List(FrustumPlanes& frustum, Nodes& nodes);
	void ListLeafs(FrustumPlanes& frustum, Nodes& nodes);

	// move all nodes into vector, set top to nullptr
	void Clear(Nodes& nodes);

	QuadNode* GetNode(const Vec2& pos, float radius);

	QuadNode* top;
	GetQuadNode get;
	Nodes tmpNodes;
};
