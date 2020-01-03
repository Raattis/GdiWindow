#include "GdiDrawing.h"

#include "Guard.h"

#include <assert.h>
#include <Windows.h>
#include <wingdi.h>
#include <vcruntime_string.h>
#include <inttypes.h>
#include <malloc.h>
#include <map>

namespace GdiWindow
{
struct WindowState
{
	HWND hwnd = nullptr;
	unsigned char *buffer = nullptr;
	HDC hDibDC;
	HGDIOBJ hgdiobj;
	int h = 0;
	int w = 0;
};

struct WindowStateMap
{
	std::map<HWND, WindowState> map;
};

Ref<WindowStateMap> getWindowStateMap()
{
	static Guard<WindowStateMap> map;
	return map;
}

struct InProgressState
{
	bool blitting = false;
	bool wantsToBlit = false;
	bool drawing = false;
	bool wantsToDraw = false;
};

Ref<InProgressState> getInProgressState(HWND hwnd)
{
	static Guard<std::map<HWND, InProgressState>> mapState;
	Ref<std::map<HWND, InProgressState>> map = mapState;
	InProgressState &state = map->operator[](hwnd);
	return stealMutex(map, state);
}

void GdiDraw::init(void *hwndParam)
{
	HWND hwnd = (HWND)hwndParam;
	Ref<WindowStateMap> map = getWindowStateMap();
	WindowState &state = map->map[hwnd];
	state.hwnd = hwnd;

	WINDOWINFO wi;
	GetWindowInfo(state.hwnd, &wi);

	int h = wi.rcClient.bottom - wi.rcClient.top;
	int w = wi.rcClient.right - wi.rcClient.left;

	if (h < 1)
		h = 1;

	if (w < 1)
		w = 1;

	state.h = h;
	state.w = w;

	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = state.w;
	bmi.bmiHeader.biHeight = state.h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	HDC hDesktopDC = GetDC(hwnd);
	HBITMAP hDib = CreateDIBSection(hDesktopDC, &bmi, DIB_RGB_COLORS, (void **)&state.buffer, 0, 0);

	if (hDib == NULL)
	{
		assert(!"hDib null");
	}

	if (state.buffer == NULL)
	{
		assert(!"buffer was null");
	}

	state.hDibDC = CreateCompatibleDC(hDesktopDC);
	state.hgdiobj = SelectObject(state.hDibDC, hDib);

	ReleaseDC(hwnd, hDesktopDC);
}

void GdiDraw::deinit(void *hwndParam)
{
	HWND hwnd = (HWND)hwndParam;
	Ref<WindowStateMap> map = getWindowStateMap();
	WindowState &state = map->map[hwnd];

	DeleteObject(state.hgdiobj);

	ReleaseDC(hwnd, state.hDibDC);
}

static void paintImpl(HWND hwnd)
{
	Ref<WindowStateMap> map = getWindowStateMap();
	WindowState &state = map->map[hwnd];

	// GdiFlush();

	PAINTSTRUCT paint;
	HDC hwndDc = BeginPaint(hwnd, &paint);
	BitBlt(hwndDc, 0, 0, state.w, state.h, state.hDibDC, 0, 0, SRCCOPY);
	EndPaint(hwnd, &paint);
}

void GdiDraw::paint(void *hwndParam)
{
	HWND hwnd = (HWND)hwndParam;

	{
		Ref<InProgressState> inProgressState = getInProgressState(hwnd);
		if (inProgressState->blitting || inProgressState->wantsToBlit)
			return;

		inProgressState->wantsToBlit = true;

		while (inProgressState->drawing)
		{
			InverseMutexGuard ig(*inProgressState.m);
			sleep(1);
		}

		inProgressState->blitting = true;
		inProgressState->wantsToBlit = false;
	}

	paintImpl(hwnd);

	{
		Ref<InProgressState> inProgressState = getInProgressState(hwnd);
		inProgressState->blitting = false;
	}
}

static void drawImpl(unsigned char *buffer, int w, int h)
{
	static int v = 0;
	if (v++ % 50)
	{
		return;
	}

	static int c = 0;

	for (int i = 0, end = w * h * 4; i < end; i += 4)
	{
		++c;
		buffer[i + 0] = (c / 16) % 255;
		buffer[i + 1] = (c / 4) % 125;
		buffer[i + 2] = (c / 4) % 30;
		buffer[i + 3] = 255;
	}
}

void GdiDraw::draw(void *hwndParam, const GdiDrawInfo &info)
{
	HWND hwnd = (HWND)hwndParam;

	assert(getInProgressState(hwnd)->drawing);

	Ref<WindowStateMap> map = getWindowStateMap();
	WindowState &state = map->map[hwnd];
	drawImpl(state.buffer, state.w, state.h);
}

void GdiDraw::beginDrawing(void *hwndParam)
{
	HWND hwnd = (HWND)hwndParam;
	Ref<InProgressState> inProgressState = getInProgressState(hwnd);
	assert(!inProgressState->drawing);
	assert(!inProgressState->wantsToDraw);

	inProgressState->wantsToDraw = true;

	while (inProgressState->blitting || inProgressState->wantsToBlit)
	{
		InverseMutexGuard ig(*inProgressState.m);
		sleep(10);
	}

	inProgressState->drawing = true;
	inProgressState->wantsToDraw = false;
}

void GdiDraw::endDrawing(void *hwndParam)
{
	HWND hwnd = (HWND)hwndParam;
	getInProgressState(hwnd)->drawing = false;
}


}
