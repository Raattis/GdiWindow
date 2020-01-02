#include "Window.h"
#include "GdiDrawing.h"

#include <thread>
#include <chrono>

using namespace GdiWindow;

static void initGdiDraw(const WindowHandle &windowHandle, void *hwnd)
{
	GdiDraw::init(hwnd);
}

static void deinitGdiDraw(const WindowHandle &windowHandle, void *hwnd)
{
	GdiDraw::deinit(hwnd);
}

static void paintGdiDraw(const WindowHandle &windowHandle, void *hwnd)
{
	GdiDraw::paint(hwnd);
}

static void moved(const WindowHandle &windowHandle, void *hwnd)
{
}

static int64_t message(const WindowHandle &windowHandle, void *hwnd, uint32_t msg, uint64_t wParam, int64_t lParam)
{
	return 0;
}

int main()
{
	WindowHandle h("asdf");
	Window::registerStartedDelegate(h, &initGdiDraw);
	Window::registerStoppingDelegate(h, &deinitGdiDraw);
	Window::registerPaintDelegate(h, &paintGdiDraw);
	Window::registerMoveDelegate(h, &moved);
	Window::registerMessageDelegate(h, &message);

	Window::open(h);

	while (Window::exists(h))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		printf(".");
	}

	return 0;
}
