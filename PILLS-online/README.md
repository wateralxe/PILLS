
# 项目简介

本项目为供需分配问题的 C++ 实现，主要用于将离线分配结果与线上需求进行高效再分配，适用于广告、流量等资源分配场景。核心流程包括数据读取、索引映射、分配优化与结果输出。

## 目录结构

- `src/`
	- `file_reader.hpp`/`file_reader.cpp`：数据结构与文件读取工具，负责将供给、需求、分配结果等文件解析为高效索引结构。
	- `main.cpp`：主程序入口，包含分配主流程、贪心分配与 PV 移动等核心算法。
	- `Makefile`：编译脚本，支持一键编译生成可执行文件 `main`。

## 编译方法

在 `src/` 目录下执行：

```bash
make
```

生成的可执行文件为 `main`。

## 使用方法

命令行参数如下：

```bash
./main <demand_file> <supply_file> <offline_res_csv> <online_demand_file> <online_result_csv> <cutoff_time>
```

- `<demand_file>`：需求文件
- `<supply_file>`：供给文件
- `<offline_res_csv>`：离线分配结果
- `<online_demand_file>`：线上需求文件
- `<online_result_csv>`：输出线上分配结果
- `<cutoff_time>`：优化循环的最大秒数（int，单位：秒），控制分配优化的运行时长。

## 核心功能说明

- **IdMapper**：字符串与整数 ID 的双向映射，提升索引效率。
- **read_xxx_indexed**：一系列文件读取函数，将原始数据转为高效索引结构。
- **move_pv/greedy_allocate_online**：核心分配与 PV 移动算法，支持贪心与局部搜索优化。
- **主循环**：在限定时间内多次尝试优化分配，最终输出合并后的分配结果。

## 适用场景

适合大规模供需匹配、广告流量分配、资源再分配等需要高效索引和批量处理的场景。

---

如需更详细的函数注释或示例数据说明，请告知！
