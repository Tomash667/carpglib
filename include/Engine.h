#pragma once

//-----------------------------------------------------------------------------
#include "Timer.h"

//-----------------------------------------------------------------------------
class Engine
{
	friend class Render;
public:
	static const Int2 MIN_WINDOW_SIZE;
	static const Int2 DEFAULT_WINDOW_SIZE;

	Engine();
	~Engine();

	void LoadConfiguration(Config& cfg);
	bool Start();
	void Shutdown();
	void DoPseudotick(bool msgOnly = false);
	void RestoreFocus();
	void ShowError(cstring msg, Logger::Level level = Logger::L_ERROR);
	void FatalError(cstring err);
	void LockCursor();
	void UnlockCursor(bool lockOnFocus = true);
	void HideWindow(bool hide);

	bool IsActive() const { return active; }
	bool IsCursorLocked() const { return lockedCursor; }
	bool IsCursorVisible() const { return cursorVisible; }
	bool IsShutdown() const { return shutdown; }
	bool IsFullscreen() const { return fullscreen; }

	const Int2& GetClientSize() const { return clientSize; }
	float GetFps() const { return fps; }
	float GetWindowAspect() const { return float(clientSize.x) / clientSize.y; }
	HWND GetWindowHandle() const { return hwnd; }
	const Int2& GetWindowSize() const { return wndSize; }
	CustomCollisionWorld* GetPhysicsWorld() { return phyWorld; }

	void SetFullscreen(bool fullscreen);
	void SetTitle(cstring title);
	void SetUnlockPoint(const Int2& pt) { unlockPoint = pt; }
	void SetWindowInitialPos(const Int2& pos, const Int2& size) { forcePos = pos; forceSize = size; }
	void SetWindowSize(const Int2& size);
	void ToggleFullscreen() { SetFullscreen(!IsFullscreen()); }

private:
	void Init();
	void AdjustWindowSize();
	void Cleanup();
	void DoTick(bool updateGame);
	long HandleEvent(HWND hwnd, uint msg, uint wParam, long lParam);
	bool MsgToKey(uint msg, uint wParam, byte& key, int& result);
	void InitWindow();
	void PlaceCursor();
	void ShowCursor(bool show);
	void UpdateActivity(bool isActive);
	void WindowLoop();
	bool IsWindowActive();
	void SetWindowSizeInternal(const Int2& size);

	CustomCollisionWorld* phyWorld;
	HWND hwnd;
	Timer timer;
	string title;
	Int2 wndSize, realSize, clientSize, unlockPoint, activationPoint, forcePos, forceSize;
	float frameTime, fps;
	uint frames;
	bool initialized, shutdown, cursorVisible, replaceCursor, lockedCursor, lockOnFocus, active, fullscreen, hiddenWindow, inResize;
};
