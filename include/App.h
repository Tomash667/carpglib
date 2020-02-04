#pragma once

//-----------------------------------------------------------------------------
class App
{
public:
	App();
	virtual ~App();
	int Start();

	virtual bool OnInit() { return true; }
	virtual void OnCleanup() {}
	virtual void OnReload() {}
	virtual void OnReset() {}
	virtual void OnResize() {}
	virtual void OnUpdate(float dt) {}
	virtual void OnFocus(bool focus, const Int2& activation_point) {}

protected:
	Engine* engine;
};
