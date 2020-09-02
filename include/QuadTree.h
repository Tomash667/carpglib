#pragma once

struct QuadTree
{
	struct Node
	{
		Box2d box;
		Node* childs[4];
		bool leaf;
	};

	typedef vector<Node*> Nodes;

	QuadTree() : root(nullptr) {}
	void Init(delegate<Node*()> get, const Box2d& box, int splits);
	bool List(const FrustumPlanes& frustum, delegate<void(Node*)> callback);
	void ListLeafs(const FrustumPlanes& frustum, delegate<void(Node*)> callback);
	void Clear(delegate<void(Node*)> callback);
	Node* GetNode(const Vec2& pos, float radius);
	Node* GetRoot() { return root; }
	void ForEach(delegate<void(Node*)> callback);
	bool IsInitialized() { return root != nullptr; }


private:
	void Init(Node* node, const Box2d& box, int splits);

	delegate<Node*()> get;
	Node* root;
	Nodes tmpNodes;
};
