#include "GdiTypes.h"

#include <chrono>
#include <thread>

namespace GdiWindow
{

void sleepImpl(float ms)
{
	std::this_thread::sleep_for(std::chrono::microseconds(int(ms * 1000)));
}


}
