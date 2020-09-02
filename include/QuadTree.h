#pragma once

struct QuadNode
{
	Box2d box;
	QuadNode* childs[4];
	bool leaf;
};

typedef QuadNode* (*GetQuadNode)();

struct QuadTree
{
	typedef vector<QuadNode*> Nodes;

	QuadTree() : top(nullptr) {}
	void Init(QuadNode* node, const Box2d& box,  int splits);
	void List(FrustumPlanes& frustum, Nodes& nodes);
	void ListLeafs(FrustumPlanes& frustum, Nodes& nodes);
	void Clear(Nodes& nodes);
	QuadNode* GetNode(const Vec2& pos, float radius);

	QuadNode* top;
	GetQuadNode get;
	Nodes tmp_nodes;
};
