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

	FIXME;
	//bool ChangeMode(bool fullscreen);
	//bool ChangeMode(Int2 size, bool fullscreen, uint hz = 0);
	void DoPseudotick(bool msg_only = false);
	void Shutdown();
	void FatalError(cstring err);
	void ShowError(cstring msg, Logger::Level level = Logger::L_ERROR);
	bool Start();
	void UnlockCursor(bool lock_on_focus = true);
	void LockCursor();
	void HideWindow(bool hide);

	bool IsActive() const { return active; }
	bool IsCursorLocked() const { return locked_cursor; }
	bool IsCursorVisible() const { return cursor_visible; }
	bool IsShutdown() const { return shutdown; }
	bool IsFullscreen() const { return fullscreen; }

	float GetFps() const { return fps; }
	float GetWindowAspect() const { return float(client_size.x) / client_size.y; }
	HWND GetWindowHandle() const { return hwnd; }
	const Int2& GetWindowSize() const { return client_size; }
	CustomCollisionWorld* GetPhysicsWorld() { return phy_world; }

	void SetFullscreen(bool fullscreen);
	void SetTitle(cstring title);
	void SetUnlockPoint(const Int2& pt) { unlock_point = pt; }
	void SetWindowInitialPos(const Int2& pos, const Int2& size) { force_pos = pos; force_size = size; }
	void SetWindowSize(const Int2& size);

private:
	void Init();
	void AdjustWindowSize();
	//void ChangeMode();
	FIXME;
	void Cleanup();
	void DoTick(bool update_game);
	long HandleEvent(HWND hwnd, uint msg, uint wParam, long lParam);
	bool MsgToKey(uint msg, uint wParam, byte& key, int& result);
	void InitWindow();
	void PlaceCursor();
	void ShowCursor(bool show);
	void UpdateActivity(bool is_active);
	void WindowLoop();
	bool IsWindowActive();
	void SetWindowSizeInternal(const Int2& size);

	CustomCollisionWorld* phy_world;
	HWND hwnd;
	Timer timer;
	string title;
	Int2 wnd_size, real_size, client_size, unlock_point, activation_point, force_pos, force_size;
	float frame_time, fps;
	uint frames;
	bool initialized, shutdown, cursor_visible, replace_cursor, locked_cursor, lock_on_focus, active, fullscreen, hidden_window, in_resize;
};
