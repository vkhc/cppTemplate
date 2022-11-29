#include "timer.h"

#include <iostream>
#include <thread>
#include <future>
#include <algorithm>
#include <execution>
#include <cassert>
#include <sstream>

struct myNum {
	myNum(int n = 0) : number(n) {}

	auto operator<=>(const myNum& rhs) const = default;
	myNum& operator+=(const myNum& other)
	{
		number += other.number;
		return *this;
	}

	int number = 0;
};

myNum operator+(const myNum& lhs, const myNum& rhs)
{
	std::this_thread::sleep_for(std::chrono::nanoseconds(50));
	return myNum{ lhs.number + rhs.number };
}


template<typename iterator>
std::vector<std::pair<iterator, iterator>> getDivisions(iterator begin, iterator end, unsigned nDivisions)
{
	std::vector<std::pair<iterator, iterator>> output;

	size_t d = std::distance(begin, end);
	size_t chunkSize = d / nDivisions;

	auto chunkBegin = begin;
	auto chunkEnd = begin;

	while (chunkEnd != end) {
		chunkBegin = chunkEnd;
		if (d - std::distance(begin, chunkEnd) < chunkSize * 1.1)
			chunkEnd = end;
		else
			std::advance(chunkEnd, chunkSize);
		output.emplace_back(std::pair<iterator, iterator>(chunkBegin, chunkEnd));

	}

	return output;
}

template<typename inputIt, typename outputIt>
void my_partial_sum(inputIt begin, inputIt end, outputIt outBegin)
{
	typename std::iterator_traits<inputIt>::value_type sum = *begin;
	*outBegin = sum;

	while (++begin != end) {
		sum = std::move(sum) + *begin;
		*++outBegin = sum;
	}

	
}

template<typename inputIt, typename outputIt>
bool sub_partial_sum(inputIt begin, inputIt end, outputIt outBegin, typename std::iterator_traits<inputIt>::value_type& lastVal)
{
	using valType = std::iterator_traits<inputIt>::value_type;
	valType sum = *begin;
	*outBegin = sum;

	while (++begin != end) {
		sum = std::move(sum) + *begin;
		*++outBegin = sum;
	}

	lastVal = std::move(sum);
	
	return true;
}

template<typename T>
bool check_vector(const T& v)
{
	for (const auto& i : v)
		if (i == false)
			return false;
	// std::cout << "checking values: ";
	// for (const auto& i : v)
	// 	std::cout << (int)i << ' ';
	// std::cout << '\n';
	return true;
}

template<typename inputIt, typename outputIt>
void p_partial_sum(inputIt begin, inputIt end, outputIt outBegin)
{
	int nThreads = 16;
	// Divide Array into subarrays
	auto chunks = getDivisions(begin, end, nThreads);

	using valType = std::iterator_traits<inputIt>::value_type;

	std::vector<valType> increments(chunks.size() + 1);
	std::vector<std::atomic<bool>> increments_done(chunks.size());
	for (auto& value : increments_done)
		value = false;
	bool readyToIncrement = false;

	std::mutex m;
	std::condition_variable cv;
	std::vector<std::thread> threads;
	threads.reserve(chunks.size());


	for (int i = 0; i < chunks.size(); ++i) {
		auto inFirst = chunks[i].first;
		auto inLast = chunks[i].second;
		auto outFirst = outBegin + std::distance(begin, inFirst);

		threads.emplace_back([&readyToIncrement, &increments, &increments_done, &m, &cv, inFirst, inLast, outFirst, i]() {
			
			increments_done[i] = sub_partial_sum<inputIt, outputIt>(inFirst, inLast, outFirst, increments[i + 1]);

			std::unique_lock lk(m);
			cv.wait(lk, [&]{ return readyToIncrement; });
			auto val = increments[i];
			// std::cout << "Thread " << std::this_thread::get_id() << " incrementing\n";
			for_each(outFirst, outFirst + std::distance(inFirst, inLast), [&](auto& x) { x += val; });

		});

	}

	/* Problem Summary:
		In debug works as expected, in release by adding artificial delaysd only*/

	//std::this_thread::sleep_for(std::chrono::seconds(1));
	while (true) {
		if (check_vector(increments_done) == true) {
			std::unique_lock lk(m);
									// std::cout << "DBG: ";
									// for (const auto& i : increments_done)
									// 	std::cout << (int)i << ' ';
									// std::cout << '\n';
			std::partial_sum(increments.begin(), increments.end(), increments.begin());
			readyToIncrement = true;
			lk.unlock();
			cv.notify_all();
			break;
		}
	}

	for (auto& t : threads)
		t.join();
}

int main()
{
	std::vector<myNum> vIn(1000, random(-10000, 10000)); //= { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };//
	auto vOut = vIn;

	auto vInMy = vIn;
	auto vOutMy = vIn;

	Timer t1, t2;

	t1.set();
	std::partial_sum(vIn.begin(), vIn.end(), vOut.begin());
	t1.stop();

	t2.set();
	p_partial_sum(vInMy.begin(), vInMy.end(), vOutMy.begin());
	//std::exclusive_scan(std::execution::par, vInMy.begin(), vInMy.end(), vOutMy.begin(), vInMy.front());
	t2.stop();

	std::cout << "std partial sum sequential [ms]: " << t1.elapsed_ms() << '\n';
	std::cout << "my partial sum parallel [ms]: " << t2.elapsed_ms() << '\n';

	//  for (const auto& i : vOut)
	//  	std::cout << i << ' ';
	//  std::cout << '\n';

	//  for (const auto& i : vOutMy)
	//  	std::cout << i << ' ';
	//  std::cout << '\n';

	// assert(vOut.size() == vOutMy.size() && "Size of outputs not equal");
	for (int i = 0; i < vOut.size(); ++i)
		if (vOut[i] != vOutMy[i])
			abort();
	// 	assert(vOut[i] == vOutMy[i] && "Value not equal");

}
