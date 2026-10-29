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
