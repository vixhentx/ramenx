#include "logic.hpp"

#include <array>
#include <ramenx/logic.hpp>
#include <tuple>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(LOGIC_TEST);

namespace
{

struct IntTracer
{
	const char* name;
	ramen::Pushable<int> in = [this](const int& value) { LOG_INF("%s: %d", name, value); };
};

struct BoolTracer
{
	const char* name;
	ramen::Pushable<bool> in = [this](const bool& value) { LOG_INF("%s: %s", name, value ? "true" : "false"); };
};

void demo_property()
{
	LOG_INF("Property");
	ramen::Property<int> property{0};
	IntTracer changes{"changed"};
	property.out >> changes.in;

	// 预期：只输出 1、3，重复输入不会触发通知；最后读取到 3。
	for (const int value : {1, 1, 3, 3})
	{
		property.in(value);
	}

	int current = 0;
	property.out_cur(current);
	LOG_INF("current: %d", current);
}

void demo_picker()
{
	LOG_INF("Picker");
	ramen::Picker<int> positive = [](const int& value) { return value > 0; };
	IntTracer accepted{"accepted"};
	positive.out >> accepted.in;

	// 预期：只转发正数 2、4。
	for (const int value : {-1, 0, 2, 4})
	{
		positive.in(value);
	}

	ramen::Picker<int, int> same_parity = [](const int& left, const int& right) { return (left % 2) == (right % 2); };
	ramen::Pushable<int, int> pair_out = [](const int& left, const int& right) {
		LOG_INF("pair: (%d, %d)", left, right);
	};
	same_parity.out >> pair_out;
	// 预期：只输出 (2, 4)，(2, 3) 因奇偶性不同而被丢弃。
	same_parity.in(2, 4);
	same_parity.in(2, 3);

	ramen::Picker<> trigger = [] { return true; };
	ramen::Pushable<> trigger_out = [] { LOG_INF("triggered"); };
	trigger.out >> trigger_out;
	// 预期：每次无参数输入都触发一次输出。
	trigger.in();
}

void demo_combine()
{
	LOG_INF("Combine");
	ramen::Combine<ramen::CombineTrigger::Any, int, int, int> any(
	    [](const int& left, const int& right) { return left + right; }, 10, 20
	);
	IntTracer any_result{"any"};
	any.out >> any_result.in;
	// 预期：输入第 0 个值输出 31，输入第 1 个值输出 32。
	std::get<0>(any.in)(11); // Any: either input recomputes with the latest values.
	std::get<1>(any.in)(21);

	ramen::Combine<ramen::CombineTrigger::First, int, int, int> first(
	    [](const int& left, const int& right) { return left * right; }, 2, 3
	);
	IntTracer first_result{"first"};
	first.out >> first_result.in;
	// 预期：更新第 1 个值时不输出，更新第 0 个值时输出 20（5 * 4）。
	std::get<1>(first.in)(4); // First: this only updates the cached second input.
	std::get<0>(first.in)(5); // The first input triggers using both latest values.
}

void demo_window_and_pair()
{
	LOG_INF("Window / Pair");
	ramen::Window<int, 3> window{{1, 2, 3}};
	ramen::Pushable<std::array<int, 3>> window_out = [](const auto& values) {
		LOG_INF("window: [%d, %d, %d]", values[0], values[1], values[2]);
	};
	window.out >> window_out;
	// 预期：依次输出 [2, 3, 4]、[3, 4, 5]，窗口始终保留最新的 3 个值。
	window.in(4);
	window.in(5);

	ramen::Pair<int> pair{10};
	ramen::Pushable<std::pair<int, int>> pair_out = [](const auto& values) {
		LOG_INF("pair: (%d, %d)", values.first, values.second);
	};
	pair.out >> pair_out;
	// 预期：依次输出 (10, 11)、(11, 12)，输出后当前值成为下一次的前值。
	pair.in(11);
	pair.in(12);
}

void demo_accumulator()
{
	LOG_INF("Accumulator");
	ramen::Accumulator<int, int> sum([](const int& state, const int& input) { return state + input; }, 0);
	IntTracer result{"sum"};
	sum.out >> result.in;
	// 预期：依次输出 2、5、4，状态由前一次结果继续累加。
	sum.in(2);
	sum.in(3);
	sum.in(-1);
}

void demo_batcher()
{
	LOG_INF("Batcher");
	ramen::Batcher<int, 3> batcher;
	ramen::Pushable<std::array<int, 3>> batch_out = [](const auto& values) {
		LOG_INF("batch: [%d, %d, %d]", values[0], values[1], values[2]);
	};
	batcher.out >> batch_out;
	// 预期：输入 1、2、3 时输出 [1, 2, 3]；输入 4 暂存在下一批中，不产生输出。
	for (const int value : {1, 2, 3, 4})
	{
		batcher.in(value);
	}
}

void demo_hysteresis()
{
	LOG_INF("Hysteresis");
	ramen::Hysteresis<int> state{
	    false,
	    [](const int& value) { return value >= 10; },
	    [](const int& value) { return value <= 3; },
	};
	BoolTracer result{"state"};
	state.out >> result.in;
	// 预期：5 不切换，10 输出 true，8 保持 true，3 输出 false，5 保持 false。
	for (const int value : {5, 10, 8, 3, 5})
	{
		state.in(value);
	}
}

void demo_deadband_and_comparator()
{
	LOG_INF("Deadband / Comparator");
	ramen::Deadband<int> deadband{100, [](const int& last, const int& next) { return next - last >= 5; }};
	IntTracer accepted{"deadband"};
	deadband.out >> accepted.in;
	// 预期：只输出 105；被接受的值才会成为下一次比较基准。
	for (const int value : {102, 104, 105, 108, 109})
	{
		deadband.in(value);
	}

	ramen::Comparator<int> rising{[](const int& previous, const int& current) { return current > previous; }, 0};
	BoolTracer comparison{"rising"};
	rising.out >> comparison.in;
	// 预期：依次输出 true、false、false、true；即使结果重复也不会去重。
	for (const int value : {1, 1, 0, 2})
	{
		rising.in(value);
	}
}

void demo_gate()
{
	LOG_INF("Gate");
	ramen::Gate<int> gate;
	IntTracer result{"gate"};
	gate.out >> result.in;
	// 预期：默认关闭时丢弃 1，打开后输出 2，关闭时丢弃 3，再打开后输出 4。
	gate.in(1); // Closed by default.
	gate.in_control(true);
	gate.in(2);
	gate.in_control(false);
	gate.in(3); // Disabled inputs are discarded, not cached.
	gate.in_control(true);
	gate.in(4);
}

void demo_count_gate()
{
	LOG_INF("CountGate");
	ramen::CountGate<ramen::CountMode::Take, int> take{2};
	IntTracer taken{"take"};
	take.out >> taken.in;
	// 预期：Take 只输出前两个值 1、2，计数耗尽后丢弃 3。
	for (const int value : {1, 2, 3})
	{
		take.in(value);
	}

	ramen::CountGate<ramen::CountMode::Skip, int> skip{2};
	IntTracer skipped{"skip"};
	skip.out >> skipped.in;
	// 预期：Skip 丢弃前两个值 1、2，之后输出 3、4。
	for (const int value : {1, 2, 3, 4})
	{
		skip.in(value);
	}

	skip.in_count(1); // The counter can be reset without producing an output.
	// 预期：重设后再次跳过 5，不会产生输出。
	skip.in(5);
}

} // namespace

void logic::demo()
{
	LOG_INF("Logic playground started.");
	demo_property();
	demo_picker();
	demo_combine();
	demo_window_and_pair();
	demo_accumulator();
	demo_batcher();
	demo_hysteresis();
	demo_deadband_and_comparator();
	demo_gate();
	demo_count_gate();
	LOG_INF("Logic playground finished.");
}
