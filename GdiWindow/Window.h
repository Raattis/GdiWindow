#pragma once

#include <string>
#include <inttypes.h>
#include "GdiTypes.h"

namespace GdiWindow
{

struct WindowHandle
{
	std::string name;
	uint32_t number;

	WindowHandle() {}
	WindowHandle(const char *name, uint32_t number = 0) : name(name), number(number) {}

	bool operator<(const WindowHandle &o) const
	{
		return name < o.name || name == o.name && number < o.number;
	}
};

typedef void(*StartedDelegate)(const WindowHandle &windowHandle, void *hwnd);
typedef void(*StoppingDelegate)(const WindowHandle &windowHandle, void *hwnd);
typedef void(*PaintDelegate)(const WindowHandle &windowHandle, void *hwnd);
typedef void(*MoveDelegate)(const WindowHandle &windowHandle, void *hwnd);
typedef void(*ResizeDelegate)(const WindowHandle &windowHandle, void* hwnd);
typedef int64_t(*MessageDelegate)(const WindowHandle &windowHandle, void *hwnd, uint32_t msg, uint64_t wParam, int64_t lParam);

struct Window
{
	static void open(const WindowHandle &windowHandle);
	static void close(const WindowHandle &windowHandle);
	static bool isOpen(const WindowHandle &windowHandle);
	static bool exists(const WindowHandle &windowHandle);
	static void *getHwnd(const WindowHandle &windowHandle);

	static void repaint(const WindowHandle &windowHandle);

	static void setRect(const WindowHandle &windowHandle, Rect rect);
	static Rect getRect(const WindowHandle &windowHandle);

	static void registerStartedDelegate(const WindowHandle &windowHandle, StartedDelegate func);
	static void registerStoppingDelegate(const WindowHandle &windowHandle, StoppingDelegate func);
	static void registerPaintDelegate(const WindowHandle &windowHandle, PaintDelegate func);
	static void registerMoveDelegate(const WindowHandle &windowHandle, MoveDelegate func);
	static void registerResizeDelegate(const WindowHandle& windowHandle, ResizeDelegate func);
	static void registerMessageDelegate(const WindowHandle &windowHandle, MessageDelegate func);
};

}
