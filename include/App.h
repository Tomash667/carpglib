#pragma once

//-----------------------------------------------------------------------------
class App
{
public:
	App();
	virtual ~App();
	virtual bool Start();
	virtual bool OnInit() { return true; }
	virtual void OnCleanup() {}
	virtual void OnDraw();
	virtual void OnCustomDraw() {}
	virtual void OnResize();
	virtual void OnUpdate(float dt);
	virtual void OnFocus(bool focus, const Int2& activationPoint) {}
};
