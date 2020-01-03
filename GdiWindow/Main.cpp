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

static void resized(const WindowHandle& windowHandle, void* hwnd)
{
	GdiDraw::deinit(hwnd);
	GdiDraw::init(hwnd);
}

static int64_t message(const WindowHandle &windowHandle, void *hwnd, uint32_t msg, uint64_t wParam, int64_t lParam)
{
	return 0;
}

static void doDrawing(WindowHandle h)
{
	while (Window::exists(h))
	{
		void *hwnd = Window::getHwnd(h);
		GdiDraw::beginDrawing(hwnd);
		GdiDrawInfo info;
		GdiDraw::draw(hwnd, info);
		sleep(5);
		GdiDraw::endDrawing(hwnd);
	}
}

int main()
{
	WindowHandle h("asdf");
	Window::registerStartedDelegate(h, &initGdiDraw);
	Window::registerStoppingDelegate(h, &deinitGdiDraw);
	Window::registerPaintDelegate(h, &paintGdiDraw);
	Window::registerMoveDelegate(h, &moved);
	Window::registerResizeDelegate(h, &resized);
	Window::registerMessageDelegate(h, &message);

	Window::open(h);

	std::thread(doDrawing, h).detach();

	while (Window::exists(h))
	{
		sleep(33);
		printf(".");
		Window::repaint(h);
	}

	return 0;
}
