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

计划实现这些功能:

- [ ] 异步actor: 解决跨线程问题的工具集合
	- `MsgPipe`: 消息队列封装
	- `Dispatcher`: 跨线程执行器
	- ...
- [ ] 线性actor: 解决数据处理的工具集合
	- `Combine`: 按需组合数据
	- ...
- [ ] 响应actor: 解决时序处理的工具集合
	- `Debounce`: 数据消抖
	- ...
