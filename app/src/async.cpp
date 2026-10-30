#include "async.hpp"
#include "zephyr/sleep.h"
#include <ramen.hpp>
#include <ramenx/async.hpp>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ASYNC_TEST);

namespace
{

class Sensor
{
public:
	/**
	 * @brief 在中断上下文触发的"传感器"数据
	 *
	 */
	ramen::Pusher<int> out_isr{};

	Sensor()
	{
		k_timer_init(&timer_, tick, nullptr);
		timer_.user_data = this;
	}
	void start() { k_timer_start(&timer_, K_NO_WAIT, K_MSEC(10)); }
	Sensor(Sensor&&) = delete;
	Sensor& operator=(const Sensor&) = delete;
	Sensor& operator=(Sensor&&) = delete;

private:
	k_timer timer_{};
	atomic_t cnt_{};
	static void tick(k_timer* timer)
	{
		auto& self = reinterpret_cast<Sensor&>(timer->user_data);
		atomic_inc(&self.cnt_);
		// 中断直接输出数据
		auto value = static_cast<int>(self.cnt_);
		// FIXME: 这里会有bug! 执行时观测者列表是空的!
		self.out_isr(value);
	}
};

class Tracer
{
public:
	ramen::Pushable<int> in = [](const int& v) { Tracer::trace(v); };

private:
	static void trace(int v)
	{
		if (k_is_in_isr())
		{
			LOG_INF("Context: ISR. Sensor Value is %d", v);
		}
		else
		{
			const auto* thread_name = k_thread_name_get(k_current_get());
			LOG_INF("Context: thread '%s', sensor value is %d", thread_name ? thread_name : "null", v);
		}
	}
};

Sensor sensor{};
Tracer tracer{};
ramen::MsgQueue<int, 4> queue{};
ramen::Dispatcher<int, 2048> dispatcher({.priority = 1, .name = "sensor_dispatcher"});

} // namespace

namespace async
{

/*
 * 连接关系:
 * sensor >> queue << dispatcher >> trace
 */
void demo(k_timeout_t time)
{
	LOG_INF("Async demo started.");

	sensor.out_isr >> queue.in;           // 传感器在isr推消息队列
	dispatcher.in_wait >> queue.out_wait; // dispatcher 主动拉取消息队列
	dispatcher.out >> tracer.in;          // 传感器推送到tracer

	LOG_INF("Async actor connected.");
	dispatcher.start();
	sensor.start();

	LOG_INF("Async actor started.");

	queue.in(11111);

	k_sleep(time);
}
} // namespace async
