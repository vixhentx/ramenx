# RAMENX

ramenx 是 嵌入式消息库 [`ramen`](https://github.com/Zubax/ramen) 的Extension, 设计目标如下:

- 为 `ramen` 添加响应流算子, 简化数据处理
- 集成到 `zephyr` , 并提供调度算子

## 项目结构

本项目是zephyr T2 topography, 核心组件如下:

- modules: west自动拉取的模块. 用于开发环境
- app: 测试项目, 以及触发完整的configure流程. 用于开发环境
- src/inc: 本项目源代码. 被外部引用.
- zephyr: 用于集成到zephyr模块系统中. 被外部引用

## 设计计划与进展

### 异步 actor (`async`)

跨执行上下文传递数据, 提供队列和线程驱动能力.

- [x] `MsgQueue`: 固定容量消息队列, 支持溢出策略
- [x] `ThreadLift`: 专用线程驱动的 pull -> push 桥接
- [x] `QueueDispatcher`: 消息队列与专用处理线程的组合
- [ ] `名字没想好`: 将输出通知派发到指定执行上下文

### 逻辑 actor (`logic`)

由外部输入或读取驱动, 可保存有界状态, 默认同步执行并要求串行访问.

- [.] `Property`: 保存当前值, 支持读取和变化通知
- [ ] `Filter`: 按谓词过滤输入
- [ ] `Combine`: 多源最新值组合
- [ ] `With`: 主输入触发时, 组合其他输入的最新值
- [ ] `Window` / `Pair`: 将数据按窗口滑动组合
- [ ] `Scan`: 累积更新状态并输出
- [ ] `Buffer`: 按数量收集数据, 使用固定容量缓冲区
- [ ] `MovingAverage`: 固定窗口滑动平均
- [ ] `Hysteresis`: 双阈值滞回判断
- [ ] `Deadband`: 过滤指定幅度内的微小变化
- [ ] `Edge`: 检测布尔输入的上升沿和下降沿
- [ ] `Gate`: 根据控制输入允许或阻止数据通过
- [ ] `Take`: 转发前 N 次输入
- [ ] `Skip`: 跳过前 N 次输入

复用 ramen 已有的 `Latch`, `Lift`, `PushUnary`, `PullUnary`, `PullNary`, `PushCast` 和 `PullCast`.

### 时间 actor (`time`)

通过时间或循环驱动执行, 共享定时设施, 明确输出执行上下文.

- [ ] `TimeProvider`: 提供单调时间与定时回调, 支持 Zephyr 后端和可控测试时间
- [ ] `FrameProvider`: 提供外部循环或 tick 驱动, 支持按迭代次数处理
- [ ] `Timer`: 延迟触发一次/周期性触发
- [ ] `Debounce`: 连续静默指定时长后输出最新输入
- [ ] `Throttle`: 每个限流时间段仅转发首个输入 / 首个输入开启时间段, 到期输出该段最新输入
- [ ] `Sample`: 按周期采样最新输入
- [ ] `Delay`: 延迟输入, 使用有界缓冲区和明确的溢出策略
- [ ] `Timeout`: 指定时长内未收到输入时通知
- [ ] `Timestamp`: 为输入附加单调时间戳
- [ ] `TimeInterval`: 计算相邻输入的时间间隔
