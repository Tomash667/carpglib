#include "Pch.h"
#include "Engine.h"

#include "App.h"
#include "Config.h"
#include "Gui.h"
#include "Input.h"
#include "Physics.h"
#include "Render.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "WindowsIncludes.h"

//-----------------------------------------------------------------------------
Engine* app::engine;
const Int2 Engine::MIN_WINDOW_SIZE = Int2(800, 600);
const Int2 Engine::DEFAULT_WINDOW_SIZE = Int2(1280, 720);
constexpr int WINDOWED_FLAGS = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME;
constexpr int FULLSCREEN_FLAGS = WS_POPUP;

//=================================================================================================
Engine::Engine() : initialized(false), shutdown(false), timer(false), hwnd(nullptr), cursorVisible(true), replaceCursor(false), lockedCursor(true),
active(false), activationPoint(-1, -1), phyWorld(nullptr), title("Window"), forcePos(-1, -1), forceSize(-1, -1), hiddenWindow(false),
wndSize(DEFAULT_WINDOW_SIZE), clientSize(wndSize)
{
	if(!Logger::GetInstance())
		Logger::SetInstance(new Logger);
	app::gui = new Gui;
	app::input = new Input;
	app::render = new Render;
	app::resMgr = new ResourceManager;
	app::sceneMgr = new SceneManager;
	app::soundMgr = new SoundManager;
}

//=================================================================================================
Engine::~Engine()
{
	delete Logger::GetInstance();
}

//=================================================================================================
void Engine::LoadConfiguration(Config& cfg)
{
	// log
	PreLogger* plog = dynamic_cast<PreLogger*>(Logger::GetInstance());
	TextLogger* textLogger = nullptr;
	ConsoleLogger* consoleLogger = nullptr;
	int count = 0;

	// compatibility
	cfg.Rename("con_pos", "conPos");
	cfg.Rename("con_size", "conSize");
	cfg.Rename("feature_level", "featureLevel");
	cfg.Rename("hidden_window", "hiddenWindow");
	cfg.Rename("log_filename", "logFilename");
	cfg.Rename("multisampling_quality", "multisamplingQuality");
	cfg.Rename("music_volume", "musicVolume");
	cfg.Rename("sound_device", "soundDevice");
	cfg.Rename("sound_volume", "soundVolume");
	cfg.Rename("use_normalmap", "useNormalmap");
	cfg.Rename("use_specularmap", "useSpecularmap");
	cfg.Rename("wnd_pos", "wndPos");
	cfg.Rename("wnd_size", "wndSize");

	if(cfg.GetBool("log", true))
	{
		textLogger = new TextLogger(cfg.GetString("logFilename", "log.txt").c_str());
		++count;
	}

	if(cfg.GetBool("console"))
	{
		consoleLogger = new ConsoleLogger;
		const Int2 consolePos = cfg.GetInt2("conPos", Int2(-1, -1));
		const Int2 consoleSize = cfg.GetInt2("conSize", Int2(-1, -1));
		if(consolePos != Int2(-1, -1) || consoleSize != Int2(-1, -1))
			consoleLogger->Move(consolePos, consoleSize);
		++count;
	}

	Logger* logger;
	if(count == 2)
		logger = new MultiLogger({ textLogger, consoleLogger });
	else if(count == 1)
		logger = textLogger ? static_cast<Logger*>(textLogger) : static_cast<Logger*>(consoleLogger);
	else
		logger = new Logger;

	if(plog)
		plog->Apply(logger);
	Logger::SetInstance(logger);

	// window settings
	bool isFullscreen = cfg.GetBool("fullscreen", true);
	Int2 wndSize = cfg.GetInt2("resolution");
	Info("Settings: Resolution %dx%d (%s).", wndSize.x, wndSize.y, isFullscreen ? "fullscreen" : "windowed");
	SetFullscreen(isFullscreen);
	SetWindowSize(wndSize);
	SetWindowInitialPos(cfg.GetInt2("wndPos", Int2(-1, -1)), cfg.GetInt2("wndSize", Int2(-1, -1)));
	HideWindow(cfg.GetBool("hiddenWindow"));

	// render settings
	int adapter = cfg.GetInt("adapter");
	Info("Settings: Adapter %d.", adapter);
	app::render->SetAdapter(adapter);
	const string& featureLevel = cfg.GetString("featureLevel");
	if(!featureLevel.empty())
	{
		if(!app::render->SetFeatureLevel(featureLevel))
			Warn("Settings: Invalid feature level '%s'.", featureLevel.c_str());
	}
	app::render->SetVsync(cfg.GetBool("vsync", true));
	app::render->SetMultisampling(cfg.GetInt("multisampling"), cfg.GetInt("multisamplingQuality"));
	app::sceneMgr->useNormalmap = cfg.GetBool("useNormalmap", true);
	app::sceneMgr->useSpecularmap = cfg.GetBool("useSpecularmap", true);

	// sound/music settings
	if(cfg.GetBool("nosound"))
		app::soundMgr->Disable();
	app::soundMgr->SetSoundVolume(Clamp(cfg.GetInt("soundVolume", 100), 0, 100));
	app::soundMgr->SetMusicVolume(Clamp(cfg.GetInt("musicVolume", 50), 0, 100));
	Guid soundDevice;
	if(soundDevice.TryParse(cfg.GetString("soundDevice", Guid::EmptyString)))
		app::soundMgr->SetDevice(soundDevice);
}

//=================================================================================================
// Adjust window size to take exact value
void Engine::AdjustWindowSize()
{
	if(!fullscreen)
	{
		Rect rect = Rect::Create(Int2::Zero, wndSize);
		AdjustWindowRect((RECT*)&rect, WINDOWED_FLAGS, false);
		realSize = rect.Size();
	}
	else
		realSize = wndSize;
}

//=================================================================================================
// Cleanup engine
void Engine::Cleanup()
{
	Info("Engine: Cleanup.");

	app::app->OnCleanup();

	delete app::input;
	delete app::resMgr;
	delete app::render;
	delete app::gui;
	delete app::sceneMgr;
	delete app::soundMgr;

	CustomCollisionWorld::Cleanup(phyWorld);
}

//=================================================================================================
// Do pseudo update tick, used to render in update loop
void Engine::DoPseudotick(bool msgOnly)
{
	MSG msg = { 0 };
	if(!timer.IsStarted())
		timer.Start();

	while(msg.message != WM_QUIT && PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if(msgOnly)
		timer.Tick();
	else
		DoTick(false);
}

//=================================================================================================
// Call after returning from native dialog
void Engine::RestoreFocus()
{
	DoPseudotick(true);
	app::input->ReleaseKeys();
	app::input->UpdateShortcuts();
}

//=================================================================================================
// Common part for WindowLoop and DoPseudotick
void Engine::DoTick(bool updateGame)
{
	const float dt = timer.Tick();
	assert(dt >= 0.f);

	// calculate fps
	frames++;
	frameTime += dt;
	if(frameTime >= 1.f)
	{
		fps = frames / frameTime;
		frames = 0;
		frameTime = 0.f;
	}

	// update activity state
	bool isActive = IsWindowActive();
	bool wasActive = active;
	UpdateActivity(isActive);

	// handle cursor movement
	Int2 mouseDif = Int2::Zero;
	if(active)
	{
		if(lockedCursor)
		{
			if(replaceCursor)
				replaceCursor = false;
			else if(wasActive)
			{
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				mouseDif = Int2(pt.x, pt.y) - clientSize / 2;
			}
			PlaceCursor();
		}
	}
	else if(!lockedCursor && lockOnFocus)
		lockedCursor = true;
	app::input->UpdateMouseDif(mouseDif);

	// update keyboard shortcuts info
	app::input->UpdateShortcuts();

	// update game
	if(updateGame)
		app::app->OnUpdate(dt);
	if(shutdown)
	{
		if(active && lockedCursor)
		{
			Rect rect;
			GetClientRect(hwnd, (RECT*)&rect);
			Int2 wh = rect.Size();
			POINT pt;
			pt.x = int(float(unlockPoint.x) * wh.x / wndSize.x);
			pt.y = int(float(unlockPoint.y) * wh.y / wndSize.y);
			ClientToScreen(hwnd, &pt);
			SetCursorPos(pt.x, pt.y);
		}
		return;
	}
	app::input->UpdateMouseWheel(0);

	app::app->OnDraw();
	app::input->Update();
	app::soundMgr->Update(dt);
}

//=================================================================================================
bool Engine::IsWindowActive()
{
	HWND foreground = GetForegroundWindow();
	if(foreground != hwnd)
		return false;
	return !IsIconic(hwnd);
}

//=================================================================================================
// Start closing engine
void Engine::Shutdown()
{
	if(!shutdown)
	{
		shutdown = true;
		Info("Engine: Started closing engine...");
	}
}

//=================================================================================================
// Show fatal error
void Engine::FatalError(cstring err)
{
	assert(err);
	ShowError(err, Logger::L_FATAL);
	Shutdown();
}

//=================================================================================================
// Handle windows events
long Engine::HandleEvent(HWND inHwnd, uint msg, uint wParam, long lParam)
{
	switch(msg)
	{
	// window closed/destroyed
	case WM_CLOSE:
	case WM_DESTROY:
		shutdown = true;
		return 0;

	// window size change
	case WM_SIZE:
		if(wParam == SIZE_MAXIMIZED)
			SetFullscreen(true);
		else if(wParam != SIZE_MINIMIZED)
		{
			if(!inResize)
			{
				RECT rect = {};
				GetWindowRect((HWND)hwnd, &rect);
				realSize = Int2(rect.right - rect.left, rect.bottom - rect.top);

				GetClientRect((HWND)hwnd, &rect);
				clientSize = Int2(rect.right - rect.left, rect.bottom - rect.top);
			}

			app::render->OnChangeResolution();
			if(initialized)
				app::app->OnResize();
		}
		return 0;

	// handle keyboard
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		app::input->Process((Key)wParam, true);
		return 0;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		app::input->Process((Key)wParam, false);
		return 0;

	// handle mouse
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
		{
			byte key;
			int result;
			bool down = MsgToKey(msg, wParam, key, result);

			if((!lockedCursor || !active) && down && lockOnFocus)
			{
				ShowCursor(false);
				Rect rect;
				GetClientRect(hwnd, (RECT*)&rect);
				Int2 wh = rect.Size();
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				activationPoint = Int2(pt.x * wndSize.x / wh.x, pt.y * wndSize.y / wh.y);
				PlaceCursor();

				if(active)
					lockedCursor = true;

				return result;
			}

			app::input->Process((Key)key, down);
			return result;
		}

	// close alt+space menu
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	// handle text input
	case WM_CHAR:
	case WM_SYSCHAR:
		app::gui->OnChar((char)wParam);
		return 0;

	// handle mouse wheel
	case WM_MOUSEWHEEL:
		app::input->UpdateMouseWheel(app::input->GetMouseWheel() + float(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA);
		return 0;
	}

	// return default message
	return DefWindowProc(inHwnd, msg, wParam, lParam);
}

//=================================================================================================
// Convert message to virtual key
bool Engine::MsgToKey(uint msg, uint wParam, byte& key, int& result)
{
	bool down = false;

	switch(msg)
	{
	default:
		assert(0);
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		down = true;
	case WM_LBUTTONUP:
		key = VK_LBUTTON;
		result = 0;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		down = true;
	case WM_RBUTTONUP:
		key = VK_RBUTTON;
		result = 0;
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		down = true;
	case WM_MBUTTONUP:
		key = VK_MBUTTON;
		result = 0;
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONDBLCLK:
		down = true;
	case WM_XBUTTONUP:
		key = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
		result = TRUE;
		break;
	}

	return down;
}

//=================================================================================================
// Create window
void Engine::InitWindow()
{
	wndSize = Int2::Max(wndSize, MIN_WINDOW_SIZE);

	// register window class
	HMODULE module = GetModuleHandle(nullptr);
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		[](HWND hwnd, uint msg, WPARAM wParam, LPARAM lParam) -> LRESULT { return app::engine->HandleEvent(hwnd, msg, wParam, lParam); },
		0, 0, module, LoadIcon(module, "Icon"), LoadCursor(nullptr, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH),
		nullptr, "Krystal", nullptr
	};
	if(!RegisterClassEx(&wc))
		throw Format("Failed to register window class (%d).", GetLastError());

	// create window
	AdjustWindowSize();
	hwnd = CreateWindowEx(0, "Krystal", title.c_str(), WINDOWED_FLAGS, 0, 0, realSize.x, realSize.y,
		nullptr, nullptr, module, nullptr);
	if(!hwnd)
		throw Format("Failed to create window (%d).", GetLastError());

	// position window
	if(fullscreen)
	{
		ShowWindow(hwnd, SW_SHOWNORMAL);
		fullscreen = false;
		SetFullscreen(true);
	}
	else
	{
		if(forcePos != Int2(-1, -1) || forceSize != Int2(-1, -1))
		{
			// set window position from config file
			Rect rect;
			GetWindowRect(hwnd, (RECT*)&rect);
			if(forcePos.x != -1)
				rect.Left() = forcePos.x;
			if(forcePos.y != -1)
				rect.Top() = forcePos.y;
			Int2 size = realSize;
			if(forceSize.x != -1)
				size.x = forceSize.x;
			if(forceSize.y != -1)
				size.y = forceSize.y;
			SetWindowPos(hwnd, 0, rect.Left(), rect.Top(), size.x, size.y, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
		else
		{
			// set window at center of screen
			MoveWindow(hwnd,
				(GetSystemMetrics(SM_CXSCREEN) - realSize.x) / 2,
				(GetSystemMetrics(SM_CYSCREEN) - realSize.y) / 2,
				realSize.x, realSize.y, false);
		}
	}

	// show window
	ShowWindow(hwnd, hiddenWindow ? SW_HIDE : SW_SHOWNORMAL);

	// reset cursor
	replaceCursor = true;
	app::input->UpdateMouseDif(Int2::Zero);
	unlockPoint = realSize / 2;

	Info("Engine: Window created.");
}

//=================================================================================================
// Place cursor on window center
void Engine::PlaceCursor()
{
	POINT destPt = { clientSize.x / 2, clientSize.y / 2 };
	ClientToScreen(hwnd, &destPt);
	SetCursorPos(destPt.x, destPt.y);
}

//=================================================================================================
// Change window title
void Engine::SetTitle(cstring title)
{
	assert(title);
	this->title = title;
	if(initialized)
		SetWindowTextA(hwnd, title);
}

//=================================================================================================
// Show/hide cursor
void Engine::ShowCursor(bool show)
{
	if(IsCursorVisible() != show)
	{
		::ShowCursor(show);
		cursorVisible = show;
	}
}

//=================================================================================================
// Show error
void Engine::ShowError(cstring msg, Logger::Level level)
{
	assert(msg);

	ShowWindow(hwnd, SW_HIDE);
	ShowCursor(true);
	Logger* logger = Logger::GetInstance();
	logger->Log(level, msg);
	logger->Flush();
	MessageBox(nullptr, msg, nullptr, MB_OK | MB_ICONERROR | MB_APPLMODAL);
	RestoreFocus();
}

//=================================================================================================
// Initialize and start engine
bool Engine::Start()
{
	// initialize engine
	try
	{
		Init();
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Failed to initialize engine!\n%s", e), Logger::L_FATAL);
		Cleanup();
		return false;
	}

	// initialize game
	try
	{
		if(!app::app->OnInit())
		{
			Cleanup();
			return false;
		}
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Failed to initialize app!\n%s", e), Logger::L_FATAL);
		Cleanup();
		return false;
	}

	// loop game
	try
	{
		if(lockedCursor && active)
			PlaceCursor();
		WindowLoop();
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Game error!\n%s", e));
		Cleanup();
		return false;
	}

	// cleanup
	Cleanup();
	return true;
}

//=================================================================================================
void Engine::Init()
{
	InitWindow();
	app::render->Init();
	app::soundMgr->Init();
	phyWorld = CustomCollisionWorld::Init();
	app::resMgr->Init();
	app::gui->Init();
	app::sceneMgr->Init();
	initialized = true;
}

//=================================================================================================
// Unlock cursor - show system cursor and allow to move outside of window
void Engine::UnlockCursor(bool lockOnFocus)
{
	this->lockOnFocus = lockOnFocus;
	if(!lockedCursor)
		return;
	lockedCursor = false;

	if(!IsCursorVisible())
	{
		Rect rect;
		GetClientRect(hwnd, (RECT*)&rect);
		Int2 wh = rect.Size();
		POINT pt;
		pt.x = int(float(unlockPoint.x) * wh.x / wndSize.x);
		pt.y = int(float(unlockPoint.y) * wh.y / wndSize.y);
		ClientToScreen(hwnd, &pt);
		SetCursorPos(pt.x, pt.y);
	}

	ShowCursor(true);
}

//=================================================================================================
// Lock cursor when window gets activated
void Engine::LockCursor()
{
	if(lockedCursor)
		return;
	lockOnFocus = true;
	lockedCursor = true;
	if(active)
	{
		ShowCursor(false);
		PlaceCursor();
	}
}

//=================================================================================================
void Engine::HideWindow(bool hide)
{
	if(hide == hiddenWindow)
		return;
	hiddenWindow = hide;
	if(initialized)
		ShowWindow(hwnd, hide ? SW_HIDE : SW_SHOWNORMAL);
}

//=================================================================================================
// Update window activity
void Engine::UpdateActivity(bool isActive)
{
	if(isActive == active)
		return;
	active = isActive;
	if(active)
	{
		if(lockedCursor)
		{
			ShowCursor(false);
			PlaceCursor();
		}
	}
	else
	{
		ShowCursor(true);
		app::input->ReleaseKeys();
	}
	app::app->OnFocus(active, activationPoint);
	activationPoint = Int2(-1, -1);
}

//=================================================================================================
// Main window loop
void Engine::WindowLoop()
{
	MSG msg = { 0 };

	// start timer
	timer.Start();
	frames = 0;
	frameTime = 0.f;
	fps = 0.f;

	while(msg.message != WM_QUIT)
	{
		// handle winapi messages
		if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
			DoTick(true);

		if(shutdown)
			break;
	}
}

//=================================================================================================
void Engine::SetWindowSizeInternal(const Int2& size)
{
	wndSize = size;
	AdjustWindowSize();
	SetWindowPos(hwnd, HWND_NOTOPMOST, (GetSystemMetrics(SM_CXSCREEN) - realSize.x) / 2, (GetSystemMetrics(SM_CYSCREEN) - realSize.y) / 2,
		realSize.x, realSize.y, SWP_SHOWWINDOW | SWP_DRAWFRAME);
}


//=================================================================================================
void Engine::SetFullscreen(bool fullscreen)
{
	if(this->fullscreen == fullscreen)
		return;
	this->fullscreen = fullscreen;
	if(!hwnd)
		return;
	inResize = true;
	HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfoA(monitor, &monitorInfo);
	Rect& monitorRect = *reinterpret_cast<Rect*>(&monitorInfo.rcMonitor);
	if(fullscreen)
	{
		clientSize = monitorRect.Size();
		realSize = clientSize;
		SetWindowLong((HWND)hwnd, GWL_STYLE, FULLSCREEN_FLAGS);
		SetWindowPos((HWND)hwnd, HWND_NOTOPMOST,
			monitorRect.Left(), monitorRect.Top(),
			clientSize.x, clientSize.y,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);
		ShowWindow((HWND)hwnd, SW_MAXIMIZE);
	}
	else
	{
		clientSize = wndSize;
		AdjustWindowSize();
		SetWindowLong((HWND)hwnd, GWL_STYLE, WINDOWED_FLAGS);
		SetWindowPos((HWND)hwnd, HWND_NOTOPMOST,
			monitorRect.Left() + (monitorRect.SizeX() - realSize.x) / 2,
			monitorRect.Top() + (monitorRect.SizeY() - realSize.y) / 2,
			realSize.x, realSize.y,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);
		ShowWindow((HWND)hwnd, SW_NORMAL);
	}
	inResize = false;
}

//=================================================================================================
void Engine::SetWindowSize(const Int2& size)
{
	if(wndSize == size)
		return;
	wndSize = size;
	if(!initialized)
		return;
	if(!fullscreen)
	{
		clientSize = size;
		AdjustWindowSize();
		SetWindowPos((HWND)hwnd, HWND_NOTOPMOST,
			(GetSystemMetrics(SM_CXSCREEN) - realSize.x) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - realSize.y) / 2,
			realSize.x, realSize.y,
			SWP_NOACTIVATE);
	}
}
