
#include "file_reader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

// 主vector结构定义
IdMapper sid_mapper, did_mapper;
std::vector<int> sid_pv, sid_remainpv;
std::vector<std::unordered_set<int>> sid_did;
std::vector<std::unordered_set<int>> did_sid;
std::vector<int> did_amount;
std::vector<int> online_did;
std::vector<std::unordered_map<int, int>> sid_did_allocatepv;

// 新的索引化读取supply
void read_supply_indexed(const std::string& filepath) {
    std::ifstream fin(filepath);
    std::string line;
    while (std::getline(fin, line)) {
        std::stringstream ss(line);
        std::string aid, pid, pv_total_str, demands_str;
        std::getline(ss, aid, '`');
        std::getline(ss, pid, '`');
        std::getline(ss, pv_total_str, '`');
        std::getline(ss, demands_str, '`');
        std::string sid = aid + "_" + pid;
        int sid_idx = sid_mapper.get_or_add(sid);
        int pv_total = std::stoi(pv_total_str);
        // 保证所有 sid 相关 vector 足够大
        if ((int)sid_pv.size() <= sid_idx) sid_pv.resize(sid_idx+1, 0);
        if ((int)sid_remainpv.size() <= sid_idx) sid_remainpv.resize(sid_idx+1, 0);
        if ((int)sid_did.size() <= sid_idx) sid_did.resize(sid_idx+1);
        if ((int)sid_did_allocatepv.size() <= sid_idx) sid_did_allocatepv.resize(sid_idx+1);
        sid_pv[sid_idx] += pv_total;
        sid_remainpv[sid_idx] += pv_total;
        std::stringstream dss(demands_str);
        std::string demand_id;
        while (std::getline(dss, demand_id, ',')) {
            int did_idx = did_mapper.get_or_add(demand_id);
            // 保证所有 did 相关 vector 足够大
            if ((int)did_amount.size() <= did_idx) did_amount.resize(did_idx+1, 0);
            if ((int)online_did.size() <= did_idx) online_did.resize(did_idx+1, 0);
            if ((int)did_sid.size() <= did_idx) did_sid.resize(did_idx+1);
            sid_did[sid_idx].insert(did_idx);
            did_sid[did_idx].insert(sid_idx);
        }
    }
}

// 新的索引化读取demand
void read_demand_indexed(const std::string& filepath) {
    std::ifstream fin(filepath);
    std::string line;
    while (std::getline(fin, line)) {
        std::stringstream ss(line);
        std::string demand_id, pv_needed_str;
        std::getline(ss, demand_id, '`');
        std::getline(ss, pv_needed_str, '`');
        int did_idx = did_mapper.get(demand_id); // 只处理 supply 阶段注册过的 demand
        if (did_idx == -1) continue; // 未注册的直接丢弃
        did_amount[did_idx] = std::stoi(pv_needed_str);
    }
}

// 读取 online demand，只记录有效 demand 的索引到 online_did
void read_online_demand_indexed(const std::string& filepath) {
    std::ifstream fin(filepath);
    std::string line;
    online_did.clear();
    while (std::getline(fin, line)) {
        std::stringstream ss(line);
        std::string demand_id, pv_needed_str;
        std::getline(ss, demand_id, '`');
        std::getline(ss, pv_needed_str, '`');
        int did_idx = did_mapper.get(demand_id); // 只处理 supply 阶段注册过的 demand
        if (did_idx == -1) continue; // 未注册的直接丢弃
        online_did.push_back(did_idx);
    }
}

void read_result_indexed(const std::string& filepath) {
    std::ifstream fin(filepath);
    std::string line;
    // 跳过表头（如果存在）
    if (!std::getline(fin, line)) return;
    auto trim = [](std::string &s) {
        // trim spaces and tabs
        size_t start = 0;
        while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) ++start;
        size_t end = s.size();
        while (end > start && (s[end-1] == ' ' || s[end-1] == '\t' || s[end-1] == '\r' || s[end-1] == '\n')) --end;
        s = s.substr(start, end-start);
    };
    auto unquote = [](std::string &s) {
        if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
            s = s.substr(1, s.size()-2);
        }
    };
    int lineno = 1; // header line consumed
    while (std::getline(fin, line)) {
        ++lineno;
        if (line.empty()) continue; // skip blank lines
        std::stringstream ss(line);
        std::string sid_str, did_str, pv_str;
        std::getline(ss, sid_str, ',');
        std::getline(ss, did_str, ',');
        std::getline(ss, pv_str, ',');
        trim(sid_str); trim(did_str); trim(pv_str);
        unquote(sid_str); unquote(did_str); unquote(pv_str);
        // 如果某列为空，打印更详细的错误信息以便排查
        if (sid_str.empty() || did_str.empty() || pv_str.empty()) {
            std::cerr << "[错误] 在 result 文件(" << filepath << ") 第 " << lineno << " 行解析到空字段。原始行: '" << line << "'\n";
            std::cerr << "       解析得到: sid='" << sid_str << "', demand='" << did_str << "', pv='" << pv_str << "'\n";
            std::cerr << "       请检查文件分隔符、引号或是否包含额外逗号。" << std::endl;
            exit(2);
        }
        int sid = sid_mapper.get(sid_str);
        int did = did_mapper.get(did_str);
        if (sid == -1) {
            std::cerr << "[错误] result 文件 supply_id '" << sid_str << "' 未在 supply 阶段注册! (文件: " << filepath << ", 行: " << lineno << ")" << std::endl;
            std::cerr << "       原始行: '" << line << "'" << std::endl;
            exit(2);
        }
        if (did == -1) {
            std::cerr << "[错误] result 文件 demand_id '" << did_str << "' 未在 demand 阶段注册! (文件: " << filepath << ", 行: " << lineno << ")" << std::endl;
            std::cerr << "       原始行: '" << line << "'" << std::endl;
            exit(2);
        }
        int pv = 0;
        try {
            pv = std::stoi(pv_str);
        } catch (...) {
            std::cerr << "[错误] 无法将 pv 字段转换为整数: '" << pv_str << "' (文件: " << filepath << ", 行: " << lineno << ")" << std::endl;
            std::cerr << "       原始行: '" << line << "'" << std::endl;
            exit(2);
        }
        sid_remainpv[sid] -= pv;
        if (sid_remainpv[sid] < 0) {
            std::cerr << "[错误] supply " << sid_str << " 已分配超出其总pv! 剩余: " << sid_remainpv[sid] << std::endl;
            exit(2);
        }
        sid_did_allocatepv[sid][did] += pv;
    }
}
