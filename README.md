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

- [x] `Property`: 保存当前值, 支持读取和变化通知
- [x] `Picker`: 谓词(检验器)为 true 时转发输入
- [x] `Combine`: 组合最新值并输出单个结果, 编译期选择 Any 或 First 输入触发模式
- [x] `Window` / `Pair`: 输出滑动窗口(N > 2)或前后值对, 显式提供初始窗口或前值
- [x] `Accumulator`: 从显式初值累积更新状态, 每次输入均输出
- [x] `Batcher`: 使用固定容量缓冲区, 每收满 N 个输入输出一个批次(N > 1)
- [ ] `MovingAverage`: 固定窗口滑动平均
- [x] `Hysteresis`: 用 enter / leave 两个谓词控制状态切换, 仅变化时输出
- [x] `Deadband`: 谓词比较最近转发值与新输入, 为 true 时更新基准并转发
- [x] `Comparator`: 比较前值与当前输入, 每次更新前值并输出 bool
- [x] `Gate`: 默认关闭, 根据控制输入放行数据, 不缓存或补发
- [x] `CountGate`: 编译期选择 Take 或 Skip 模式, 转发或跳过指定次数的输入

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
