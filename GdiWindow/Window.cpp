#include "Window.h"

#include "Guard.h"

#include <assert.h>
#include <chrono>
#include <mutex>
#include <thread>
#include <map>
#include <vector>
#include <cstdlib>
#include <sstream>

#include <Windows.h>

namespace GdiWindow
{

struct OpenClose
{
	uint32_t open;
	uint32_t close;
};

struct OpenCloseMap
{
	std::map<WindowHandle, OpenClose> map;
};

static Ref<OpenCloseMap> getOpenCloseMap()
{
	static Guard<OpenCloseMap> map;
	return map;
}

struct DelegateState
{
	WindowHandle windowHandle;
	HWND hwnd = nullptr;

	StartedDelegate startedDelegate = nullptr;
	StoppingDelegate stoppingDelegate = nullptr;
	PaintDelegate paintDelegate = nullptr;
	MoveDelegate moveDelegate = nullptr;
	ResizeDelegate resizeDelegate = nullptr;
	MessageDelegate messageDelegate = nullptr;
};

static Ref<std::map<WindowHandle, DelegateState>> getDelegateStateMap()
{
	static Guard<std::map<WindowHandle, DelegateState>> map;
	return map;
}

static Ref<DelegateState> getDelegateState(const WindowHandle &windowHandle)
{
	Ref<std::map<WindowHandle, DelegateState>> map = getDelegateStateMap();
	return stealMutex(map, map.t->operator[](windowHandle));
}

static OptionalRef<DelegateState> tryFindDelegateStateWithHwnd(HWND hwnd)
{
	Ref<std::map<WindowHandle, DelegateState>> map = getDelegateStateMap();
	for (auto it : *map.t)
	{
		if (it.second.hwnd == hwnd)
			return stealMutexOptional(map, map->operator[](it.first));
	}

	return OptionalRef<DelegateState>();
}

typedef void(*DelegateType)(const WindowHandle &windowHandle, void *hwnd);
static void callDelegate(Ref<DelegateState> &state, DelegateType delegateObject, bool eatRef = true)
{
	if (!delegateObject)
		return;

	HWND hwnd = state->hwnd;
	WindowHandle windowHandle = state->windowHandle;

	if (eatRef)
	{
		state.m->unlock();
		state.m = nullptr;
		state.t = nullptr;
		delegateObject(windowHandle, hwnd);
	}
	else
	{
		InverseMutexGuard ig(*state.m);
		delegateObject(windowHandle, hwnd);
	}
}

struct WindowThreadState
{
	bool hasOpened = false;
	bool isClosing = false;
	bool closingSelf = false;
	bool closingExternally = false;

	HWND hwnd = nullptr;
};

struct WindowThreadStateMap
{
	std::map<WindowHandle, Guard<WindowThreadState> *> map;
};

static Ref<WindowThreadStateMap> getMap()
{
	static Guard<WindowThreadStateMap> map;
	return map;
}

static OptionalRef<WindowThreadState> tryFindWithHwnd(HWND hwnd)
{
	Ref<WindowThreadStateMap> map = getMap();
	for (auto it : map->map)
	{
		Ref<WindowThreadState> state = *it.second;
		if (state->hwnd == hwnd)
			return state;
	}

	return OptionalRef<WindowThreadState>();
}

static OptionalRef<WindowThreadState> tryGetState(const WindowHandle &windowHandle)
{
	Ref<WindowThreadStateMap> map = getMap();
	auto it = map->map.find(windowHandle);
	if (it != map->map.end())
	{
		return OptionalRef<WindowThreadState>(*it->second);
	}
	
	return OptionalRef<WindowThreadState>();
}

static Ref<WindowThreadState> getState(const WindowHandle &windowHandle)
{
	Ref<WindowThreadStateMap> map = getMap();
	auto *ptr = map->map[windowHandle];
	assert(ptr);
	return *ptr;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int64_t result = 0;
	if (OptionalRef<DelegateState> delegateStateOptional = tryFindDelegateStateWithHwnd(hwnd))
	{
		Ref<DelegateState> delegateState = dereferenceAndEat(delegateStateOptional);

		if (delegateState->messageDelegate)
		{
			MessageDelegate messageDelegate = delegateState->messageDelegate;
			WindowHandle windowHandle = delegateState->windowHandle;
			InverseMutexGuard ig(*delegateState.m);
			messageDelegate(windowHandle, hwnd, msg, wParam, lParam);
		}
		
		if (result == 0)
		{
			if (msg == WM_PAINT)
			{
				if (delegateState->paintDelegate)
					callDelegate(delegateState, delegateState->paintDelegate);

				return 0;
			}
			else if (msg == WM_MOVE)
			{
				if (delegateState->moveDelegate)
					callDelegate(delegateState, delegateState->moveDelegate);
			}
			else if (msg == WM_SIZE)
			{
				if (delegateState->resizeDelegate)
					callDelegate(delegateState, delegateState->resizeDelegate);
			}
		}

	}

	switch (msg)
	{

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return result;

	case WM_DESTROY:
		PostQuitMessage(0);
		if (OptionalRef<WindowThreadState> state = tryFindWithHwnd(hwnd))
			state->isClosing = state->closingSelf = true;

		return result;
	}

	if (result == 0)
		return DefWindowProc(hwnd, msg, wParam, lParam);

	return result;
}

static HWND openWindow(const std::string &titleParam)
{
	static uint64_t counter = 0;
	std::ostringstream classNameTemp;
	classNameTemp << titleParam << "___" << ++counter;
	
	std::string classNameTemp2 = classNameTemp.str();
	const char *title = titleParam.c_str();
	const char *className = classNameTemp2.c_str();

	//wchar_t title[512];
	//wchar_t className[512];
	//mbstowcs_s(nullptr, className, classNameTemp.str().c_str(), 512);
	//mbstowcs_s(nullptr, title, titleParam.c_str(), 512);

	HINSTANCE hInstance = GetModuleHandle(NULL);
	if (!hInstance)
	{
		assert(!"No hInstance");
		return nullptr;
	}

	// Registering the Window Class
	WNDCLASSEXA wc;
	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = className;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassExA(&wc))
	{
		assert(!"Window Registration Failed!");
		return nullptr;
	}

	// Creating the Window
	HWND hwnd = CreateWindowExA(
		WS_EX_CLIENTEDGE,
		className,
		title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
		NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		assert(!"Window Creation Failed!");
		return nullptr;
	}

	int nCmdShow = SW_SHOWNORMAL;

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	return hwnd;
}

static void deleteState(const WindowHandle &windowHandle)
{
	Ref<WindowThreadStateMap> map = getMap();
	Guard<WindowThreadState> *statePtr = map->map[windowHandle];
	map->map.erase(windowHandle);
	statePtr->m.lock();
	statePtr->m.unlock();
	delete statePtr;
}

static void windowThread(WindowHandle windowHandle)
{
	HWND hwnd = nullptr;

	{
		std::ostringstream title;
		title << windowHandle.name;
		if (windowHandle.number > 0)
			title << " " << windowHandle.number;

		hwnd = openWindow(title.str());
		if (!hwnd)
		{
			deleteState(windowHandle);
			return;
		}

		Ref<WindowThreadStateMap> map = getMap();
		Ref<WindowThreadState> state = *map->map[windowHandle];
		state->hasOpened = true;
		state->hwnd = hwnd;
	}

	{
		Ref<DelegateState> state = getDelegateState(windowHandle);
		state->windowHandle = windowHandle;
		state->hwnd = hwnd;
		callDelegate(state, state->startedDelegate);
	}

	while (true)
	{
		// Message loop
		MSG msg;
		while (PeekMessage(&msg, hwnd, 0, 0, 0) != 0)
		{
			if (GetMessage(&msg, hwnd, 0, 0) > 0)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		sleep(10);

		bool closingRequested = false;
		{
			Ref<OpenCloseMap> openCloseMap = getOpenCloseMap();
			OpenClose &openClose = openCloseMap->map[windowHandle];
			if (openClose.open <= openClose.close)
				closingRequested = true;
		}

		bool willClose = false;
		
		{
			Ref<WindowThreadState> state = getState(windowHandle);
			if (state->closingSelf)
			{
				willClose = true;
			}
			else if (closingRequested)
			{
				state->closingExternally = true;
				state->isClosing = true;
				willClose = true;
			}
		}

		if (willClose)
			break;
	}

	{
		Ref<DelegateState> state = getDelegateState(windowHandle);
		callDelegate(state, state->stoppingDelegate, false);
		state->hwnd = nullptr;
	}

	CloseWindow(hwnd);

	deleteState(windowHandle);
}


void Window::open(const WindowHandle &windowHandle)
{
	bool mightAlreadyBeOpen = false;
	{
		Ref<OpenCloseMap> openCloseMap = getOpenCloseMap();
		OpenClose &openClose = openCloseMap->map[windowHandle];
		openClose.open += 1;
		if (openClose.open <= openClose.close)
		{
			// Should stay closed
			return;
		}
		else if (openClose.open + 1 > openClose.close)
		{
			// Already open
			mightAlreadyBeOpen = true;
		}
		else
		{
			// Will open
		}
	}

	Ref<WindowThreadStateMap> map = getMap();

	{
		// Wait until the previous instance of this window is closed
		uint32_t wait = 1;
		while (1)
		{
			auto it = map->map.find(windowHandle);
			if (it == map->map.end())
				break;


			if (mightAlreadyBeOpen)
			{
				Ref<WindowThreadState> windowThreadState = *it->second;
				if (!windowThreadState->isClosing)
					return;
			}
			else
			{
				assert((*it->second)->isClosing);
			}

			map.m->unlock();

			sleep(wait);
			wait++;

			map.m->lock();
		}
	}

	Guard<WindowThreadState> *statePtr = new Guard<WindowThreadState>;
	Ref<WindowThreadState> state = *statePtr;
	map->map[windowHandle] = statePtr;
	std::thread(windowThread, windowHandle).detach();
}

void Window::close(const WindowHandle &windowHandle)
{
	Ref<OpenCloseMap> openCloseMap = getOpenCloseMap();
	openCloseMap->map[windowHandle].close += 1;
}

bool Window::isOpen(const WindowHandle &windowHandle)
{
	Ref<OpenCloseMap> openCloseMap = getOpenCloseMap();
	OpenClose &openClose = openCloseMap->map[windowHandle];
	return openClose.open > openClose.close;
}

bool Window::exists(const WindowHandle &windowHandle)
{
	Ref<WindowThreadStateMap> map = getMap();
	return map->map.find(windowHandle) != map->map.end();
}

void *Window::getHwnd(const WindowHandle &windowHandle)
{
	Ref<WindowThreadStateMap> map = getMap();
	auto it = map->map.find(windowHandle);
	if (it != map->map.end())
	{
		Ref<WindowThreadState> windowThreadState = *it->second;
		return windowThreadState->hwnd;
	}

	return nullptr;
}


void Window::repaint(const WindowHandle& windowHandle)
{
	HWND hwnd = nullptr;
	if (OptionalRef<WindowThreadState> state = tryGetState(windowHandle))
	{
		hwnd = state->hwnd;
	}

	if (hwnd)
		RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
}

void Window::setRect(const WindowHandle &windowHandle, Rect rect)
{

}

Rect Window::getRect(const WindowHandle &windowHandle)
{
	return Rect();
}

void Window::registerStartedDelegate(const WindowHandle &windowHandle, StartedDelegate func)
{
	getDelegateState(windowHandle)->startedDelegate = func;
}

void Window::registerStoppingDelegate(const WindowHandle &windowHandle, StoppingDelegate func)
{
	getDelegateState(windowHandle)->stoppingDelegate = func;
}

void Window::registerPaintDelegate(const WindowHandle &windowHandle, PaintDelegate func)
{
	getDelegateState(windowHandle)->paintDelegate = func;
}

void Window::registerMoveDelegate(const WindowHandle& windowHandle, MoveDelegate func)
{
	getDelegateState(windowHandle)->moveDelegate = func;
}

void Window::registerResizeDelegate(const WindowHandle& windowHandle, ResizeDelegate func)
{
	getDelegateState(windowHandle)->resizeDelegate = func;
}

void Window::registerMessageDelegate(const WindowHandle &windowHandle, MessageDelegate func)
{
	getDelegateState(windowHandle)->messageDelegate = func;
}

}

