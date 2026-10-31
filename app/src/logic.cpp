#include "ramen.hpp"
#include <logic.hpp>
#include <ramenx/logic.hpp>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(LOGIC_TEST);

namespace
{

struct Tracer
{
	explicit(false) Tracer(const char* name)
	    : name_(name)
	{}
	ramen::Pushable<int> in = [this](const int& value) { LOG_INF("%s value = %d", name_, value); };
	const char* name_;
};

ramen::Filter<int> odd_filter = [](const int& value) { return value % 2 == 0; };
Tracer odd_tracer = "odd";

ramen::Property<int> prop = 0;
Tracer prop_tracer = "prop";

} // namespace

void logic::demo()
{
	prop.out >> prop_tracer.in;
	odd_filter.out >> odd_tracer.in;

	// 测试odd_filter
	for (auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9})
	{
		odd_filter.in(i);
	}

	// 测试prop
	for (auto i : {1, 1, 3, 3, 5, 5})
	{
		prop.in(i);
	}
}
