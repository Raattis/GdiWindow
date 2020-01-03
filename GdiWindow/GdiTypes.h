#pragma once

namespace GdiWindow
{

struct Vec2
{
	float x = 0, y = 0;
};

struct Rect
{
	Vec2 pos, size;
};

struct Col
{
	Col(float r, float g, float b, float a = 1)
		: r(r), g(g), b(b), a(a)
	{
	}
	
	Col()
	{
	}

	float r = 0, g = 0, b = 0, a = 1;


	static Col black() { return Col(0, 0, 0, 1); }
	static Col white() { return Col(1, 1, 1, 1); }
};

void sleepImpl(float ms);

template<typename T>
inline void sleep(T ms) { sleepImpl(float(ms)); }

}
