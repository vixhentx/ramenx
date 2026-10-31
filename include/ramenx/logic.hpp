/**
 * @file logic.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 外部输入驱动的同步逻辑 actor, 要求串行访问.
 * @version 0.1
 * @date 2026-10-31
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <array>
#include <cstddef>
#include <ramen.hpp>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ramen
{
/**
 * @brief 保存当前值, 仅在值变化时推送.
 * @tparam T 支持相等比较的数据类型.
 */
template <typename T>
class Property
{
public:
	/// @brief 更新当前值.
	ramen::Pushable<T> in = [this](const T& new_v) {
		// FIXME: 明确浮点相等比较策略.
		if (new_v != value_)
		{
			value_ = new_v;
			out(new_v);
		}
	};
	/// @brief 推送变化后的值.
	ramen::Pusher<T> out{};
	/// @brief 读取当前值.
	ramen::Pullable<T> out_cur = [this](T& out) { out = value_; };

	explicit(false) Property(const T& initial)
	    : value_(initial)
	{}

private:
	T value_;
};

/**
 * @brief predicate(const T&...) 返回 true 时原样转发, 支持无参数触发.
 * @note Picker<Footprint<N>, T...> 可调整按值持有的 predicate 捕获空间.
 */
template <typename... T>
struct Picker;

template <std::size_t fp, typename... T>
struct Picker<Footprint<fp>, T...>
{
	Pushable<Footprint<sizeof(std::tuple<std::array<std::byte, fp>, void*>)>, T...> in;
	Pusher<T...> out{};

	template <typename F>
	    requires(std::is_invocable_r_v<bool, F&, const T&...>)
	explicit(false) Picker(F predicate)
	    : in([this, predicate_ = std::move(predicate)](const T&... values) mutable {
		      if (predicate_(values...))
		      {
			      out(values...);
		      }
	      })
	{}
};

template <typename... T>
struct Picker : public Picker<Footprint<default_behavior_footprint>, T...>
{
	using Picker<Footprint<default_behavior_footprint>, T...>::Picker;
};

template <std::size_t fp>
struct Picker<Footprint<fp>, void> : public Picker<Footprint<fp>>
{
	using Picker<Footprint<fp>>::Picker;
};

/// @brief 最新值组合的触发策略.
enum class CombineTrigger : char
{
	Any,   ///< 任一输入触发.
	First, ///< 仅第 0 个输入触发.
};

/**
 * @brief 按触发策略组合最新值并输出 Out, 不比较新旧值.
 * @note 构造参数为 (combine_fn, initial_inputs...), 所有初值必须显式提供.
 *       std::get<I>(in) 为第 I 个输入, 非触发输入只更新缓存.
 *       Combine<trigger, Footprint<N>, Out, In...> 指定函数捕获空间.
 */
template <CombineTrigger Trigger, typename Out, typename... In>
struct Combine;

template <CombineTrigger trigger, std::size_t fp, typename Out, typename... In>
struct Combine<trigger, Footprint<fp>, Out, In...>
{
	static_assert(sizeof...(In) > 0, "at least one input is required");

	std::tuple<In...> value;
	Function<Out(const In&...), fp> combine;
	std::tuple<Pushable<In>...> in;
	Pusher<Out> out{};

	template <typename F>
	    requires(std::is_invocable_r_v<Out, F&, const In&...>)
	explicit Combine(F fun, In... initial)
	    : Combine(std::index_sequence_for<In...>{}, std::move(fun), std::move(initial)...)
	{}

private:
	template <std::size_t... Is, typename F>
	Combine(std::index_sequence<Is...>, F fun, In... initial)
	    : value(std::move(initial)...),
	      combine(std::move(fun)),
	      in{([this](const In& incoming) { receive<Is>(incoming); })...}
	{}

	template <std::size_t I>
	void receive(const std::tuple_element_t<I, std::tuple<In...>>& incoming)
	{
		std::get<I>(value) = incoming;
		if constexpr (trigger == CombineTrigger::Any || I == 0)
		{
			// 快照隔离同步重入对缓存的修改.
			const auto snapshot = value;
			if constexpr (std::is_void_v<Out>)
			{
				std::apply(combine, snapshot);
				out();
			}
			else
			{
				const Out result = std::apply(combine, snapshot);
				out(result);
			}
		}
	}
};

template <CombineTrigger trigger, typename Out, typename... In>
struct Combine : public Combine<trigger, Footprint<default_behavior_footprint>, Out, In...>
{
	using Combine<trigger, Footprint<default_behavior_footprint>, Out, In...>::Combine;
};

/**
 * @brief 每次输入后输出从旧到新排列的 N 元素窗口.
 * @note N > 2, 显式提供完整初始窗口, 更新复杂度为 O(N).
 */
template <typename T, std::size_t N>
    requires(N > 2)
struct Window
{
	std::array<T, N> value;
	Pushable<T> in = [this](const T& incoming) {
		const T next = incoming;
		for (std::size_t i = 1; i < N; ++i)
		{
			value[i - 1] = value[i];
		}
		value[N - 1] = next;
		const auto snapshot = value;
		out(snapshot);
	};
	Pusher<std::array<T, N>> out{};

	explicit Window(std::array<T, N> initial)
	    : value(std::move(initial))
	{}
};

/**
 * @brief 显式提供初始前值, 每次输入输出 (前值, 当前值).
 */
template <typename T>
struct Pair
{
	T previous;
	Pushable<T> in = [this](const T& incoming) {
		const std::pair<T, T> snapshot{previous, incoming};
		previous = snapshot.second;
		out(snapshot);
	};
	Pusher<std::pair<T, T>> out{};

	explicit Pair(T initial)
	    : previous(std::move(initial))
	{}
};

/**
 * @brief 用 reducer(const State&, const T&) -> State 更新状态, 每次输入均输出.
 * @note 构造参数为 (reducer, initial_state).
 *       Accumulator<Footprint<N>, State, T> 指定函数捕获空间.
 */
template <typename State, typename... T>
struct Accumulator;

template <std::size_t fp, typename State, typename T>
struct Accumulator<Footprint<fp>, State, T>
{
	State value;
	Pushable<Footprint<sizeof(std::tuple<std::array<std::byte, fp>, void*>)>, T> in;
	Pusher<State> out{};

	template <typename F>
	    requires(std::is_invocable_r_v<State, F&, const State&, const T&>)
	Accumulator(F reducer, State initial)
	    : value(std::move(initial)),
	      in([this, reducer_ = std::move(reducer)](const T& incoming) mutable {
		      const State previous = value;
		      const T input = incoming;
		      const State next = reducer_(previous, input);
		      value = next;
		      out(next);
	      })
	{}
};

template <typename State, typename... T>
struct Accumulator : public Accumulator<Footprint<default_behavior_footprint>, State, T...>
{
	using Accumulator<Footprint<default_behavior_footprint>, State, T...>::Accumulator;
};

/**
 * @brief 每收满 N 个新值输出一个数组, 然后重新收集.
 * @note N > 1, 固定容量, T 需要支持默认构造与复制.
 */
template <typename T, std::size_t N>
    requires(N > 1)
struct Batcher
{
	std::array<T, N> value{};
	std::size_t count = 0;
	Pushable<T> in = [this](const T& incoming) {
		value[count] = incoming;
		++count;
		if (count == N)
		{
			const auto snapshot = value;
			count = 0;
			out(snapshot);
		}
	};
	Pusher<std::array<T, N>> out{};
};

/**
 * @brief 双 predicate 滞回判断, 仅在状态切换时输出.
 * @note 构造参数为 (initial_state, enter, leave). false 时检查 enter, true 时检查 leave,
 *       对应 predicate 返回 true 时切换状态. 每次输入最多切换一次.
 *       Hysteresis<Footprint<N>, T> 为每个 predicate 分别提供 N 字节捕获空间.
 */
template <typename... T>
struct Hysteresis;

template <std::size_t Fp, typename T>
struct Hysteresis<Footprint<Fp>, T>
{
	bool value;
	Function<bool(const T&), Fp> enter;
	Function<bool(const T&), Fp> leave;
	Pushable<T> in = [this](const T& incoming) {
		const bool previous = value;
		const T input = incoming;
		const bool next = previous ? !leave(input) : enter(input);
		if (next != previous)
		{
			value = next;
			out(next);
		}
	};
	Pusher<bool> out{};

	template <typename Enter, typename Leave>
	    requires(std::is_invocable_r_v<bool, Enter&, const T&> && std::is_invocable_r_v<bool, Leave&, const T&>)
	Hysteresis(bool initial, Enter enter_predicate, Leave leave_predicate)
	    : value(initial),
	      enter(std::move(enter_predicate)),
	      leave(std::move(leave_predicate))
	{}
};

template <typename... T>
struct Hysteresis : public Hysteresis<Footprint<default_behavior_footprint>, T...>
{
	using Hysteresis<Footprint<default_behavior_footprint>, T...>::Hysteresis;
};

/**
 * @brief 死区判断. accept(last_emitted, incoming) 为 true 时更新基准并转发, 否则保留基准.
 * @note 构造参数为 (initial_value, predicate), 初值作为基准.
 *       Deadband<Footprint<N>, T> 指定 predicate 捕获空间.
 */
template <typename... T>
struct Deadband;

template <std::size_t fp, typename T>
struct Deadband<Footprint<fp>, T>
{
	T value;
	Pushable<Footprint<sizeof(std::tuple<std::array<std::byte, fp>, void*>)>, T> in;
	Pusher<T> out{};

	template <typename F>
	    requires(std::is_invocable_r_v<bool, F&, const T&, const T&>)
	Deadband(T initial, F predicate)
	    : value(std::move(initial)),
	      in([this, accept_ = std::move(predicate)](const T& incoming) mutable {
		      const T previous = value;
		      const T next = incoming;
		      if (accept_(previous, next))
		      {
			      value = next;
			      out(next);
		      }
	      })
	{}
};

template <typename... T>
struct Deadband : public Deadband<Footprint<default_behavior_footprint>, T...>
{
	using Deadband<Footprint<default_behavior_footprint>, T...>::Deadband;
};

/**
 * @brief 每次输入计算 compare(previous, current), 更新前值后输出 bool.
 * @note 构造参数为 (compare, initial_previous), 判定结果不去重.
 *       Comparator<Footprint<N>, T> 指定比较函数捕获空间.
 */
template <typename... T>
struct Comparator;

template <std::size_t Fp, typename T>
struct Comparator<Footprint<Fp>, T>
{
	T previous;
	Pushable<Footprint<sizeof(std::tuple<std::array<std::byte, Fp>, void*>)>, T> in;
	Pusher<bool> out{};

	template <typename F>
	    requires(std::is_invocable_r_v<bool, F&, const T&, const T&>)
	Comparator(F compare, T initial)
	    : previous(std::move(initial)),
	      in([this, compare_ = std::move(compare)](const T& incoming) mutable {
		      const T old = previous;
		      const T current = incoming;
		      const bool result = compare_(old, current);
		      previous = current;
		      out(result);
	      })
	{}
};

template <typename... T>
struct Comparator : public Comparator<Footprint<default_behavior_footprint>, T...>
{
	using Comparator<Footprint<default_behavior_footprint>, T...>::Comparator;
};

/**
 * @brief 打开时转发, 关闭时丢弃, 不缓存或补发.
 * @note 默认关闭. in_control 只更新开关, 不触发输出.
 */
template <typename T>
struct Gate
{
	bool enabled;
	Pushable<bool> in_control = [this](const bool& open) { enabled = open; };
	Pushable<T> in = [this](const T& incoming) {
		if (enabled)
		{
			out(incoming);
		}
	};
	Pusher<T> out{};

	explicit Gate(bool initial = false)
	    : enabled(initial)
	{}
};

/// @brief 计数范围内的放行策略.
enum class CountMode : char
{
	Take, ///< 计数范围内转发, 耗尽后阻止.
	Skip, ///< 计数范围内跳过, 耗尽后转发.
};

/**
 * @brief 按编译期模式转发或跳过指定次数的输入, 次数默认为 0.
 * @note 输出前递减计数, 下游同步重设的次数会保留.
 */
template <CountMode Mode, typename T>
struct CountGate
{
	std::size_t remaining;
	/// @brief 覆盖剩余次数, 不触发输出.
	Pushable<std::size_t> in_count = [this](const std::size_t& count) { remaining = count; };
	Pushable<T> in = [this](const T& incoming) {
		const bool within_count = remaining > 0;
		if (within_count)
		{
			--remaining;
		}
		if constexpr (Mode == CountMode::Take)
		{
			if (within_count)
			{
				out(incoming);
			}
		}
		else
		{
			if (!within_count)
			{
				out(incoming);
			}
		}
	};
	Pusher<T> out{};

	explicit CountGate(std::size_t count = 0)
	    : remaining(count)
	{}
};

} // namespace ramen
