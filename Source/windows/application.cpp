#include "mcv_platform.h"
#include "engine.h"
#include "render/render_module.h"
#include "utils/directory_watcher.h"
#include "input/input_module.h"
#include "input/devices/device_windows.h"
#include "resources/resources.h"

LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//--------------------------------------------------------------------------------------
CApplication* CApplication::the_app = nullptr;

CApplication& CApplication::get() {
	assert(the_app);
	return *the_app;
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	// First option is given to imgui to deal with the windows msg.
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		RECT rect;
		GetClientRect(hWnd, &rect);
		CApplication::get().setDimensions(rect.right - rect.left, rect.bottom - rect.top);
		break;

	case WM_SIZING:
		break;

	case WM_SYSCOMMAND:
	{
		if (wParam == SC_KEYMENU && (lParam >> 16) <= 0) return 0;
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
	case WM_INPUT:
	{
		std::vector<input::IDevice*> devices = CEngine::get().getInput(input::PLAYER_1)->getDevices();
		for (auto device : devices)
		{
			input::IDeviceWindows* winDevice = dynamic_cast<input::IDeviceWindows*>(device);
			if (winDevice)
			{
				winDevice->processMsg(hWnd, message, wParam, lParam);
			}
		}
		break;
	}

	case CDirectoyWatcher::WM_FILE_CHANGED:
	{
		const char* filename = (const char*)(lParam);
		std::string strfilename(filename);
		dbg("File %s has changed!!\n", filename);
		// Notify everyone....
		Resources.onFileChanged(strfilename);
		CEngine::get().getModuleManager().onFileChanged(strfilename);
		delete[] filename;
		break; }

	case WM_ACTIVATEAPP:
	{
		if (wParam == FALSE)
			// Application lost focus   
		{
			PlayerInput.clearInput();
		}
	}

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

bool CApplication::init(HINSTANCE hInstance, int width, int height) {
	the_app = this;

	this->width = width;
	this->height = height;

	this->hInstance = hInstance;

	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "MCVWindowsClass";
	wcex.hIconSm = nullptr;
	if (!RegisterClassEx(&wcex))
		return false;

	// Create window
	RECT rc = { 0, 0, width, height };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	hWnd = CreateWindow("MCVWindowsClass", "EON: The Last Trial", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (!hWnd)
		return false;

	ShowWindow(hWnd, SW_SHOW);

	// Avoids jump when first delta is calculated
	centerMousePos();

	return true;
}

void CApplication::run() {

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (should_exit)
			PostMessage(hWnd, WM_CLOSE, 0, 0);

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			CEngine::get().doFrame();
		}
	}
}

void CApplication::getDimensions(int& awidth, int& aheight) const
{
	awidth = width;
	aheight = height;
}

void CApplication::setDimensions(int awidth, int aheight)
{
	width = awidth;
	height = aheight;
}

void CApplication::setMouseVisible(bool visible)
{
	mouse_visible = visible;
	if (!mouse_visible) {
		// internal counter needs to be < 0 to be hidden
		while (ShowCursor(mouse_visible) >= 0);
	}
	else {
		// internal counter needs to be >= 0 to be shown
		while (ShowCursor(mouse_visible) < 0);
	}
}

void CApplication::setMouseCentered(bool centered)
{
	mouse_centered = centered;
}

bool CApplication::getMouseHidden()
{
	return mouse_visible;
}

bool CApplication::getMouseCentered()
{
	return mouse_centered;
}

void CApplication::changeMouseState(bool state, bool center)
{
	if (!state)
	{
		// Avoids jump when first delta is calculated
		if (center) {
			centerMousePos();
		}

		setMouseCentered(true);
		setMouseVisible(false);
	}
	else
	{
		setMouseCentered(false);
		setMouseVisible(true);
	}
}