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

#include <ramen.hpp>

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
