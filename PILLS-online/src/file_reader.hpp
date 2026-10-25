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

// 主vector结构声明
extern IdMapper sid_mapper, did_mapper;
extern std::vector<int> sid_pv, sid_remainpv;
extern std::vector<std::unordered_set<int>> sid_did;
extern std::vector<std::unordered_set<int>> did_sid;
extern std::vector<int> did_amount;
extern std::vector<int> online_did;
extern std::vector<std::unordered_map<int, int>> sid_did_allocatepv;

// 新的索引化读取接口
void read_supply_indexed(const std::string& filepath);
void read_demand_indexed(const std::string& filepath);
void read_result_indexed(const std::string& filepath);
void read_online_demand_indexed(const std::string& filepath);

