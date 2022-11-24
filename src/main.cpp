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
void partial_sum_increment(inputIt begin, inputIt end, outputIt outBegin,
						   std::promise<typename std::iterator_traits<inputIt>::value_type> outValue,
						   std::shared_future<typename std::iterator_traits<inputIt>::value_type> incValue)
{
	sub_partial_sum<inputIt, outputIt>(begin, end, outBegin, std::move(outValue));

	auto val = incValue.get();
	//increment_by_val(outBegin, outBegin + std::distance(begin, end), val);
	std::for_each(outBegin, outBegin + std::distance(begin, end), [&val](auto& i) { i += val; });
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
	int nThreads = 8;
	// Divide Array into subarrays
	auto chunks = getDivisions(begin, end, nThreads);

	using valType = std::iterator_traits<inputIt>::value_type;

	std::vector<std::promise<valType>> increments(chunks.size() + 1);
	std::vector<std::shared_future<valType>> incF(increments.size());

	for (int i = 0; i < incF.size(); ++i) {
		incF[i] = increments[i].get_future();
	}
	increments[0].set_value(0);

	std::vector<std::thread> threads;
	threads.reserve(chunks.size());

	for (int i = 0; i < chunks.size(); ++i) {
		auto inFirst = chunks[i].first;
		auto inLast = chunks[i].second;
		auto outFirst = outBegin + std::distance(begin, inFirst);
		auto& promise = increments[i+1];

#if 0
		threads.emplace_back(partial_sum_increment<inputIt, outputIt>, inFirst, inLast, outFirst, std::move(promise), std::move(incF[i]));
#else

		threads.emplace_back([&promise, &incF, inFirst, inLast, outFirst, i]() {
			sub_partial_sum<inputIt, outputIt>(inFirst, inLast, outFirst, std::move(promise));

			//for (int j = 0; j <= i; ++j) {
			//	auto incVal = std::move(incF[j]).get();
			//	for_each(outFirst, outFirst + std::distance(inFirst, inLast), [&](auto& x) { x += incVal; });
			//}
		});

#endif
	}

	for (auto& t : threads)
		t.join();
}


int main()
{
	std::vector<int> vIn(1000000000, 1);
	auto vOut = vIn;

	auto vInMy = vIn;
	auto vOutMy = vIn;

	Timer t1, t2;

	t1.set();
	std::partial_sum(vIn.begin(), vIn.end(), vOut.begin());
	t1.stop();

	t2.set();
	p_partial_sum(vInMy.begin(), vInMy.end(), vOutMy.begin());
	t2.stop();

	std::cout << "std partial sum sequential [ms]: " << t1.elapsed_ms() << '\n';
	std::cout << "my partial sum parallel [ms]: " << t2.elapsed_ms();

}
