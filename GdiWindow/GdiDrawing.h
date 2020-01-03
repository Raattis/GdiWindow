#pragma once

#include "GdiTypes.h"

namespace GdiWindow
{

struct GdiDrawInfo
{
	Rect rect;
	Col col;

};

struct GdiDraw
{
	static void init(void *hwnd);
	static void deinit(void *hwnd);
	static void paint(void *hwnd);
	static void draw(void* hwnd, const GdiDrawInfo& info);
	static void beginDrawing(void* hwnd);
	static void endDrawing(void* hwnd);
};

}
