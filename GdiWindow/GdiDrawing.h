#pragma once

namespace GdiWindow
{

struct GdiDraw
{
	static void init(void *hwnd);
	static void deinit(void *hwnd);
	static void paint(void *hwnd);
};

}
