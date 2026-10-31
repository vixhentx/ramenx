/**
 * @file async.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief RTOS 跨线程工具集
 * @note 目前包含:
 * - 消息队列 MsgQueue : 线程上下文同步
 * - 派发器 Dispatcher : 专用线程调度
 * @version 0.1
 * @date 2026-10-29
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ramen.hpp>
#include <type_traits>
#include <zephyr/kernel.h>

namespace ramen
{

/**
 * @brief 消息溢出行为模式
 *
 */
enum class OverflowPolicy : std::uint8_t
{
	DropAll,      ///< 丢弃全部旧数据
	DropOldest,   ///< 丢弃最旧数据
	DropImcoming, ///< 丢弃当前数据
};

/**
 * @brief 端口化强类型消息队列. 用于跨线程间传递数据.
 *
 * @tparam T 传入数据类型
 * @tparam Capacity 消息队列大小
 * @tparam Policy 消息溢出的处理方式
 */
template <std::semiregular T, std::size_t Capacity, OverflowPolicy Policy = OverflowPolicy::DropImcoming>
    requires(Capacity > 0 && std::is_trivially_copyable_v<T>)
class MsgQueue
{
public:
	/**
	 * @brief 数据输入端口
	 *
	 */
	Pushable<T> in = [this](const T& v) { push(v); };
	/**
	 * @brief 数据读出端口(不等待)
	 *
	 */
	Pullable<std::optional<T>> out = [this](std::optional<T>& out) {
		if (T tmp; k_msgq_get(&msgq_, &tmp, K_NO_WAIT) != 0)
		{
			out.reset();
		}
		else
		{
			out.emplace(tmp);
		}
	};
	/**
	 * @brief 阻塞读取一条消息，仅允许在线程上下文中触发。
	 *
	 */
	Pullable<T> out_wait = [this](T& out) {
		while (k_msgq_get(&msgq_, &out, K_FOREVER) != 0)
		{
		}
	};

	MsgQueue() { k_msgq_init(&msgq_, msgq_buf_.data(), sizeof(T), Capacity); }
	MsgQueue(const MsgQueue&) = delete;
	MsgQueue(MsgQueue&&) = delete;

private:
	struct k_msgq msgq_{};
	std::array<char, sizeof(T) * Capacity> msgq_buf_;

	void push(const T& v) noexcept
	{
		if constexpr (Policy == OverflowPolicy::DropAll)
		{
			if (k_msgq_put(&msgq_, &v, K_NO_WAIT) != 0)
			{
				k_msgq_purge(&msgq_);
				k_msgq_put(&msgq_, &v, K_NO_WAIT);
			}
		}
		if constexpr (Policy == OverflowPolicy::DropOldest)
		{
			if (k_msgq_put(&msgq_, &v, K_NO_WAIT) != 0)
			{
				T drop;
				k_msgq_get(&msgq_, &drop, K_NO_WAIT);
				k_msgq_put(&msgq_, &v, K_NO_WAIT);
			}
		}
		if constexpr (Policy == OverflowPolicy::DropImcoming)
		{
			k_msgq_put(&msgq_, &v, K_NO_WAIT);
		}
	}
};

/**
 * @brief 线程派发器配置
 *
 */
struct DispatcherConfig
{
	int priority;               ///< 线程优先级
	const char* name = nullptr; ///< 可选线程名，需要 CONFIG_THREAD_NAME
};

/**
 * @brief 创建并持有一个线程来持续处理会阻塞等待的输入源. 借助"线程的动力"将阻塞数据源读取并推送.
 * @note 目前设计为分离线程启动与数据流连接, 并且不提供销毁(伴随整个应用)
 *
 * @tparam T 端口数据类型
 * @tparam StackSize 请求的线程栈大小(字节)
 */
template <std::semiregular T, std::size_t StackSize>
    requires(StackSize > 0)
class ThreadLift
{
public:
	/**
	 * @brief 线程阻塞等待的输入端口
	 * @note 可以连接到上面的MsgPipe等待输出端口
	 * @warning 输入应提供等待机制. 如果输入立即返回，线程会空跑循环占满CPU.
	 *
	 */
	Puller<T> in_wait{};

	/**
	 * @brief 在本线程上下文的输出处理
	 *
	 */
	Pusher<T> out{};

	explicit ThreadLift(DispatcherConfig config) noexcept
	{
		(void)k_thread_create(
		    &thread_, stack_, K_KERNEL_STACK_SIZEOF(stack_), thread_entry, this, nullptr, nullptr, config.priority, 0,
		    K_FOREVER
		);
		if (config.name != nullptr)
		{
			(void)k_thread_name_set(&thread_, config.name);
		}
	}

	~ThreadLift() noexcept
	{
		if (atomic_get(&started_) != 0)
		{
			k_thread_abort(&thread_);
		}
	}

	ThreadLift(const ThreadLift&) = delete;
	ThreadLift& operator=(const ThreadLift&) = delete;
	ThreadLift(ThreadLift&&) = delete;
	ThreadLift& operator=(ThreadLift&&) = delete;

	void start()
	{
		if (atomic_cas(&started_, 0, 1))
		{
			k_thread_start(&thread_);
		}
	};

private:
	struct k_thread thread_{};
	K_KERNEL_STACK_MEMBER(stack_, StackSize);
	atomic_t started_{};

	static void thread_entry(void* pself, void*, void*)
	{
		auto& self = *static_cast<ThreadLift*>(pself);
		for (;;)
		{
			T value{};
			self.in_wait(value);
			self.out(value);
		}
	}
};

/**
 * @brief 消息队列和处理线程二合一
 * @note 常用于将中断产生的数据调度到线程中处理. 记得start
 *
 * @tparam T 数据的类型
 * @tparam StackSize 线程栈大小
 * @tparam Capacity 消息队列容量
 * @tparam Policy 队列满时的处理方式
 */
template <
    std::semiregular T,
    std::size_t StackSize,
    std::size_t Capacity,
    OverflowPolicy Policy = OverflowPolicy::DropImcoming
>
    requires(StackSize > 0)
class QueueDispatcher
{
public:
	/**
	 * @brief 外部主动输入口. 不会等待, 可以在ISR和线程中调用.
	 *
	 */
	ramen::Pushable<T> in = [this](const T& value) { queue_.in(value); };
	/**
	 * @brief 主动输出口. 输出数据会在这个独有的线程上处理.
	 *
	 */
	ramen::Pusher<T> out;

	explicit QueueDispatcher(DispatcherConfig config)
	    : dispatcher_(config)
	{
		dispatcher_.in_wait >> queue_.out_wait;
		dispatcher_.out >> out;
	}

	void start() { dispatcher_.start(); }

	QueueDispatcher(const QueueDispatcher&) = delete;
	QueueDispatcher(QueueDispatcher&&) = delete;
	QueueDispatcher& operator=(const QueueDispatcher&) = delete;
	QueueDispatcher& operator=(QueueDispatcher&&) = delete;

private:
	MsgQueue<T, Capacity> queue_{};
	ThreadLift<T, StackSize> dispatcher_;
};

} // namespace ramen
