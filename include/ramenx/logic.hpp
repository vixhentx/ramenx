/**
 * @file logic.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 逻辑模块, 不涉及时间, 必须外部触发
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
 * @brief 属性/去重/缓存模块. 如果多次推送相同数据, 不会继续触发下游.
 *
 * @tparam T 数据类型, 需要能判断相等
 */
template <typename T>
class Property
{
public:
	/**
	 * @brief 推送式输入数据
	 *
	 */
	ramen::Pushable<T> in = [this](const T& new_v) {
		// FIXME: 处理float的精度判断问题
		if (new_v == value_)
		{
			value_ = new_v;
			out(new_v);
		}
	};
	/**
	 * @brief 在数据变化时推送数据
	 *
	 */
	ramen::Pusher<T> out{};
	/**
	 * @brief 外部主动读取口, 读取当前值
	 *
	 */
	ramen::Pullable<T> out_cur = [this](T& out) { out = value_; };

	explicit Property(T initial)
	    : value_(initial)
	{}
	explicit Property(const T& initial)
	    : value_(initial)
	{}

private:
	T value_;
};

/**
 * @brief 推送式过滤器. 判定函数返回 true 时, 将输入原样推送到下游.
 *
 * @tparam T 数据类型, 支持多参数及无参数触发.
 * @note 判定函数按值持有, 接收 const T&... 并返回可隐式转换为 bool 的结果.
 *       可用 Filter<Footprint<N>, T...> 调整判定函数的捕获空间, 不分配堆内存.
 * @code
 * Filter<int> positive = [](const int& value) { return value > 0; };
 * @endcode
 */
template <typename... T>
struct Filter;

template <std::size_t fp, typename... T>
struct Filter<Footprint<fp>, T...>
{
	Pushable<Footprint<sizeof(std::tuple<std::array<std::byte, fp>, void*>)>, T...> in;
	Pusher<T...> out{};

	template <typename F>
	    requires(std::is_invocable_r_v<bool, F&, const T&...>)
	explicit(false) Filter(F predicate)
	    : in([this, predicate_ = std::move(predicate)](const T&... values) mutable {
		      if (predicate_(values...))
		      {
			      out(values...);
		      }
	      })
	{}
};

template <typename... T>
struct Filter : public Filter<Footprint<default_behavior_footprint>, T...>
{
	using Filter<Footprint<default_behavior_footprint>, T...>::Filter;
};

template <std::size_t fp>
struct Filter<Footprint<fp>, void> : public Filter<Footprint<fp>>
{
	using Filter<Footprint<fp>>::Filter;
};

} // namespace ramen
