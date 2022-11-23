#include "timer.h"

#include <iostream>
#include <thread>
#include <future>
#include <algorithm>
#include <execution>
#include <cassert>
#include <sstream>


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

std::mutex m;

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
void sub_partial_sum(inputIt begin, inputIt end, outputIt outBegin, std::promise<typename std::iterator_traits<inputIt>::value_type> lastVal)
{
	using valType = std::iterator_traits<inputIt>::value_type;
	valType sum = *begin;
	*outBegin = sum;

	while (++begin != end) {
		sum = std::move(sum) + *begin;
		*++outBegin = sum;
	}

	lastVal.set_value(sum);
}

template<typename inputIt>
void increment_by_val(inputIt begin, inputIt end, typename std::iterator_traits<inputIt>::value_type value)
{
	for (; begin != end; ++begin)
		*begin += value;
}

template<typename inputIt, typename outputIt>
void partial_sum_increment(inputIt begin, inputIt end, outputIt outBegin, std::promise<typename std::iterator_traits<inputIt>::value_type> outValue, std::promise<typename std::iterator_traits<inputIt>::value_type> incValue)
{
	sub_partial_sum<inputIt, outputIt>(begin, end, outBegin, outValue);

	auto future = incValue.get_future();
	increment_by_val(begin, end, future.get());
}

/* Parallel partial sum
* 
* 1. Divide array in subarrays
* 2. Do partial sum on each of them concurrently
*	2.1 partial sum each sub array
*	2.2 If there are subarrays before current
* 3. increment values of subarray buy last value of previous sub array if applicable
* 
*/

template<typename inputIt, typename outputIt>
void p_partial_sum(inputIt begin, inputIt end, outputIt outBegin)
{
	int nThreads = 4;
	// Divide Array into subarrays
	auto chunks = getDivisions(begin, end, nThreads);

	using valType = std::iterator_traits<inputIt>::value_type;

	std::vector<std::promise<valType>> increments(chunks.size() + 1);

	std::vector<std::thread> threads;
	threads.reserve(chunks.size());

	for (int i = 0; i < chunks.size(); ++i) {
#if 1
		threads.emplace_back(partial_sum_increment<inputIt, outputIt>, chunks[i].first, chunks[i].second, outBegin + std::distance(begin, chunks[i].first), increments[i+1], increments[i]);
#else // bug here 
		threads.push_back([&]() {
			sub_partial_sum<inputIt, outputIt>(chunks[i].first, chunks[i].second, outBegin + std::distance(begin, chunks[i].first), increments[i]);
			});
#endif
	}

	for (auto& t : threads)
		t.join();

}


int main()
{

	std::vector<int> in = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
	auto out = in;

	p_partial_sum(in.begin(), in.end(), out.begin());

	for (auto& i : out)
		std::cout << i << ' ';
	std::cout << '\n';

	std::promise<int> x;
	sub_partial_sum(in.begin(), in.end(), out.begin(), std::move(x));
	
	std::cout << "XX " << x.get_future().get();
}
