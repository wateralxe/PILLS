// 索引化数据结构和接口声明
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>

// ID映射工具
struct IdMapper {
    std::unordered_map<std::string, int> str2id;
    std::vector<std::string> id2str;
    int get_or_add(const std::string& s) {
        auto it = str2id.find(s);
        if (it != str2id.end()) return it->second;
        int idx = id2str.size();
        str2id[s] = idx;
        id2str.push_back(s);
        return idx;
    }
    int get(const std::string& s) const {
        auto it = str2id.find(s);
        return it == str2id.end() ? -1 : it->second;
    }
    std::string to_str(int idx) const {
        return idx >= 0 && idx < (int)id2str.size() ? id2str[idx] : "";
    }
    int size() const { return id2str.size(); }
};

// 主索引化数据结构声明（离线/线上分离）
extern IdMapper sid_mapper, did_mapper;

// 离线供给，用于校验 offline result 与历史权重
extern std::vector<int> offline_sid_pv, offline_sid_remainpv;
extern std::vector<std::unordered_set<int>> offline_sid_did;
extern std::vector<std::unordered_map<int, int>> offline_sid_did_allocatepv;
extern std::vector<char> offline_sid_flag; // 1 表示此 sid 存在于离线供给

// 线上供给，用于实际 serving 抽样
extern std::vector<int> serve_sid_pv;
extern std::vector<std::unordered_set<int>> serve_sid_did;
extern std::vector<std::unordered_map<int, int>> serve_sid_did_serving_allocatepv;
extern std::vector<std::unordered_map<int, double>> serve_sid_did_prob;
extern std::vector<char> serve_sid_flag; // 1 表示此 sid 存在于线上供给

// demand 相关
extern std::vector<int> did_amount;
extern std::vector<int> online_did;

// 新的索引化读取接口（离线供给与线上供给分离）
void read_offline_supply_indexed(const std::string& filepath);
void read_serve_supply_indexed(const std::string& filepath);
void read_demand_indexed(const std::string& filepath);
void read_result_indexed(const std::string& filepath);
void read_online_demand_indexed(const std::string& filepath);

// 调试用：将已注册的 supply 和 demand 列表输出到 stderr 并写入文件
void dump_registered_mappings();

// 计算并更新每个 serve supply -> demand 的概率分布（基于 offline 分配 pv；无历史分配则在 serve 可分配集合上均匀分布），结果写入全局 serve_sid_did_prob
void compute_supply_demand_probabilities();

