#include <iostream>
#include <thread>
#include "timer.h"

int main()
{
	Timer t;
	t.set();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	t.stop();

	std::cout << "Elapsed time [s]: " << t.elapsed_s() << '\n';
	std::cout << "Elapsed time [ms]: " << t.elapsed_ms() << '\n';
	std::cout << "Elapsed time [ns]: " << t.elapsed_ns() << '\n';

}
