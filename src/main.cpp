#include "timer.h"

#include <iostream>
#include <thread>
#include <algorithm>
#include <execution>
#include <cassert>

int counter = 0;

template<typename T, typename F>
void my_for_each(T&& container, F f)
{
#if 0
	for (auto& val : container)
		f(val);
#else
	auto begin = container.begin();
	auto end = container.end();

	for (; begin != end; ++begin)
		f(*begin);
#endif

}

template<typename It, typename F>
void my_for_each(It begin, It end, F f) noexcept
{
	auto first = _Get_unwrapped(begin);
	auto last = _Get_unwrapped(end);
	for (; first != last; ++first)
		f(*first);

}

template <class _InIt, class _Fn>
constexpr void std_for_each(_InIt _First, _InIt _Last, _Fn _Func) { // perform function for each element [_First, _Last)
	_Adl_verify_range(_First, _Last);
	auto _UFirst = _Get_unwrapped(_First);
	const auto _ULast = _Get_unwrapped(_Last);
	for (; _UFirst != _ULast; ++_UFirst) {
		_Func(*_UFirst);
	}
}


int main()
{
	std::vector<int> nums1(100'000'000);

	auto function = [](int i) { i % 2; };

	
	for (auto& n : nums1)
		n = random(0, 100);

	for (int i = 0; i < 23; ++i)
		std::cout << nums1[i] << ' ';
	std::cout << '\n';

	auto nums2 = nums1;

	Timer t1;
	t1.set();
	std::for_each(nums2.begin(), nums2.end(), function);
	t1.stop();

	std::cout << "Elapsed time std [ns]: " << t1.elapsed_ns() << '\n';

	Timer t2;
	t2.set();
	my_for_each(nums1.begin(), nums1.end(), function);
	t2.stop();

	std::cout << "Elapsed time my [ns]: " << t2.elapsed_ns() << '\n';

	for (int i = 0; i < nums1.size(); ++i)
		assert(nums1[i] == nums2[i]);


}
