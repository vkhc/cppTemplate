#include "timer.h"

#include <iostream>
#include <thread>
#include <future>
#include <barrier>
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
	friend std::ostream& operator<<(std::ostream& os, const myNum& dt);

	int number = 0;
};

std::ostream& operator<<(std::ostream& os, const myNum& dt)
{
	return os << dt.number;
}

myNum operator+(const myNum& lhs, const myNum& rhs)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
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
void sub_partial_sum(inputIt begin, inputIt end, outputIt outBegin, typename std::iterator_traits<inputIt>::value_type& lastVal)
{
	using valType = std::iterator_traits<inputIt>::value_type;
	valType sum = *begin;
	*outBegin = sum;

	while (++begin != end) {
		sum = std::move(sum) + *begin;
		*++outBegin = sum;
	}

	lastVal = std::move(sum);
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
	using valType = std::iterator_traits<inputIt>::value_type;

	int nThreads = 16;
	auto chunks = getDivisions(begin, end, nThreads);

	std::vector<valType> increments(chunks.size() + 1);
	std::vector<std::jthread> threads;
	threads.reserve(chunks.size());

	std::barrier barrier(chunks.size(), [&increments]() noexcept { std::partial_sum(increments.begin(), increments.end(), increments.begin()); });

	for (int i = 0; i < chunks.size(); ++i) {
		auto inFirst = chunks[i].first;
		auto inLast = chunks[i].second;
		auto outFirst = outBegin + std::distance(begin, inFirst);

		threads.emplace_back([&barrier, &increments, inFirst, inLast, outFirst, i]() {
			
			sub_partial_sum<inputIt, outputIt>(inFirst, inLast, outFirst, increments[i + 1]);

			barrier.arrive_and_wait();

			auto val = increments[i];
			for_each(outFirst, outFirst + std::distance(inFirst, inLast), [&](auto& x) { x += val; });

		});

	}
}

int main()
{
	std::vector<myNum> vIn(1000, random(-100000, 100000));//{ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };//
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

	//for (const auto& i : vOut)
	//	std::cout << i << ' ';
	//std::cout << '\n';

	//for (const auto& i : vOutMy)
	//	std::cout << i << ' ';
	//std::cout << '\n';

	// assert(vOut.size() == vOutMy.size() && "Size of outputs not equal");
	for (int i = 0; i < vOut.size(); ++i)
		if (vOut[i] != vOutMy[i])
			abort();
	// 	assert(vOut[i] == vOutMy[i] && "Value not equal");

}
