#include <chrono>
#include <random>

class Timer {
public:
	void set();
	void stop();

	double elapsed_ns();
	double elapsed_ms();
	double elapsed_s();

private:
	std::chrono::steady_clock::time_point start;
	std::chrono::steady_clock::time_point end;
};

void Timer::set()
{
	start = std::chrono::steady_clock::now();
}

void Timer::stop()
{
	end = std::chrono::steady_clock::now();
}

double Timer::elapsed_ns()
{
	return (double)std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}
double Timer::elapsed_ms()
{
	return (double)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}
double Timer::elapsed_s()
{
	return (double)std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
}

int random(int lowerBound = INT_MIN, int upperBound = INT_MAX)
{
	static std::random_device dev;
	static std::mt19937 rng(dev());
	static std::uniform_int_distribution<std::mt19937::result_type> dist6(lowerBound, upperBound);

	return dist6(rng);
}

