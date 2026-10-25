
#include "file_reader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

// 主vector结构定义（离线/线上分离）
IdMapper sid_mapper, did_mapper;

// 离线供给
std::vector<int> offline_sid_pv, offline_sid_remainpv;
std::vector<std::unordered_set<int>> offline_sid_did;
std::vector<std::unordered_map<int, int>> offline_sid_did_allocatepv;
std::vector<char> offline_sid_flag;

// 线上供给
std::vector<int> serve_sid_pv;
std::vector<std::unordered_set<int>> serve_sid_did;
std::vector<std::unordered_map<int, int>> serve_sid_did_serving_allocatepv;
std::vector<std::unordered_map<int, double>> serve_sid_did_prob;
std::vector<char> serve_sid_flag;

// demand 相关
std::vector<int> did_amount;
std::vector<int> online_did;

namespace {
// 确保容器大小足够，便于复用
void ensure_offline_sid_capacity(int sid_idx) {
    if ((int)offline_sid_pv.size() <= sid_idx) offline_sid_pv.resize(sid_idx + 1, 0);
    if ((int)offline_sid_remainpv.size() <= sid_idx) offline_sid_remainpv.resize(sid_idx + 1, 0);
    if ((int)offline_sid_did.size() <= sid_idx) offline_sid_did.resize(sid_idx + 1);
    if ((int)offline_sid_did_allocatepv.size() <= sid_idx) offline_sid_did_allocatepv.resize(sid_idx + 1);
    if ((int)offline_sid_flag.size() <= sid_idx) offline_sid_flag.resize(sid_idx + 1, 0);
}

void ensure_serve_sid_capacity(int sid_idx) {
    if ((int)serve_sid_pv.size() <= sid_idx) serve_sid_pv.resize(sid_idx + 1, 0);
    if ((int)serve_sid_did.size() <= sid_idx) serve_sid_did.resize(sid_idx + 1);
    if ((int)serve_sid_did_serving_allocatepv.size() <= sid_idx) serve_sid_did_serving_allocatepv.resize(sid_idx + 1);
    if ((int)serve_sid_did_prob.size() <= sid_idx) serve_sid_did_prob.resize(sid_idx + 1);
    if ((int)serve_sid_flag.size() <= sid_idx) serve_sid_flag.resize(sid_idx + 1, 0);
}

void ensure_did_capacity(int did_idx) {
    if ((int)did_amount.size() <= did_idx) did_amount.resize(did_idx + 1, 0);
    if ((int)online_did.size() <= did_idx) online_did.resize(did_idx + 1, 0);
}
}

// 新的索引化读取 offline supply
void read_offline_supply_indexed(const std::string& filepath) {
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
        ensure_offline_sid_capacity(sid_idx);
        offline_sid_flag[sid_idx] = 1;
        offline_sid_pv[sid_idx] += pv_total;
        offline_sid_remainpv[sid_idx] += pv_total;
        std::stringstream dss(demands_str);
        std::string demand_id;
        while (std::getline(dss, demand_id, ',')) {
            int did_idx = did_mapper.get_or_add(demand_id);
            ensure_did_capacity(did_idx);
            offline_sid_did[sid_idx].insert(did_idx);
        }
    }
}

// 新的索引化读取 serve supply
void read_serve_supply_indexed(const std::string& filepath) {
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
        ensure_serve_sid_capacity(sid_idx);
        serve_sid_flag[sid_idx] = 1;
        serve_sid_pv[sid_idx] += pv_total;
        std::stringstream dss(demands_str);
        std::string demand_id;
        while (std::getline(dss, demand_id, ',')) {
            int did_idx = did_mapper.get_or_add(demand_id);
            ensure_did_capacity(did_idx);
            serve_sid_did[sid_idx].insert(did_idx);
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
    int skipped_over_alloc = 0; // 统计因超额分配被跳过的行
    int skipped_empty_field = 0; // 统计因空字段被跳过的行
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
            std::cerr << "[警告] 在 result 文件(" << filepath << ") 第 " << lineno << " 行解析到空字段，跳过本行。原始行: '" << line << "'\n";
            std::cerr << "        解析得到: sid='" << sid_str << "', demand='" << did_str << "', pv='" << pv_str << "'\n";
            std::cerr << "        请检查文件分隔符、引号或是否包含额外逗号。" << std::endl;
            ++skipped_empty_field;
            continue;
        }
        int sid = sid_mapper.get(sid_str);
        int did = did_mapper.get(did_str);
        if (sid == -1 || sid >= (int)offline_sid_flag.size() || !offline_sid_flag[sid]) {
            std::cerr << "[错误] result 文件 supply_id '" << sid_str << "' 未在 offline supply 中注册! (文件: " << filepath << ", 行: " << lineno << ")" << std::endl;
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
        if (sid < (int)offline_sid_did.size() && !offline_sid_did[sid].empty()) {
            if (offline_sid_did[sid].find(did) == offline_sid_did[sid].end()) {
                std::cerr << "[错误] result 文件 demand '" << did_str << "' 不在 offline supply 允许的分配集合中! (文件: " << filepath << ", 行: " << lineno << ")" << std::endl;
                std::cerr << "       原始行: '" << line << "'" << std::endl;
                exit(2);
            }
        }
        int new_remain = offline_sid_remainpv[sid] - pv;
        if (new_remain < 0) {
            std::cerr << "[警告] supply " << sid_str << " 分配超出其总pv, 跳过本行 (文件: "
                      << filepath << ", 行: " << lineno << ", 剩余: " << offline_sid_remainpv[sid]
                      << ", 待扣: " << pv << ")" << std::endl;
            ++skipped_over_alloc;
            continue; // 不修改数据结构，跳过本行
        }
        offline_sid_remainpv[sid] = new_remain;
        offline_sid_did_allocatepv[sid][did] += pv;
    }

    if (skipped_empty_field > 0) {
        std::cerr << "[警告] 共跳过 " << skipped_empty_field << " 行因空字段导致的记录" << std::endl;
    }
    if (skipped_over_alloc > 0) {
        std::cerr << "[警告] 共跳过 " << skipped_over_alloc << " 行超额分配记录，未写入历史分配" << std::endl;
    }
}

void dump_registered_mappings() {
    try {
        int offline_cnt = 0, serve_cnt = 0;
        for (char c : offline_sid_flag) if (c) ++offline_cnt;
        for (char c : serve_sid_flag) if (c) ++serve_cnt;
        std::cerr << "[DEBUG] 已注册 offline supply 数量: " << offline_cnt
                  << "，serve supply 数量: " << serve_cnt
                  << "，demand 数量: " << did_mapper.size() << std::endl;

        // 在 stderr 上输出前若干条以便快速查看
        const int preview = 50;
        std::cerr << "[DEBUG] offline supplies (前 " << preview << " 条或全部):" << std::endl;
        for (int i = 0; i < (int)sid_mapper.id2str.size() && i < preview; ++i) {
            if (i < (int)offline_sid_flag.size() && offline_sid_flag[i])
                std::cerr << "  " << i << ": " << sid_mapper.id2str[i] << std::endl;
        }
        std::cerr << "[DEBUG] serve supplies (前 " << preview << " 条或全部):" << std::endl;
        for (int i = 0; i < (int)sid_mapper.id2str.size() && i < preview; ++i) {
            if (i < (int)serve_sid_flag.size() && serve_sid_flag[i])
                std::cerr << "  " << i << ": " << sid_mapper.id2str[i] << std::endl;
        }

        std::cerr << "[DEBUG] demands (前 " << preview << " 条或全部):" << std::endl;
        for (int i = 0; i < (int)did_mapper.id2str.size() && i < preview; ++i) {
            std::cerr << "  " << i << ": " << did_mapper.id2str[i] << std::endl;
        }
        if ((int)did_mapper.id2str.size() > preview) std::cerr << "  ... (共 " << did_mapper.id2str.size() << " 条)" << std::endl;

        // 另外把完整列表写入文件，便于后续 grepping
        std::ofstream fsup_off("registered_offline_supplies.txt");
        for (int i = 0; i < (int)sid_mapper.id2str.size(); ++i) {
            if (i < (int)offline_sid_flag.size() && offline_sid_flag[i]) fsup_off << sid_mapper.id2str[i] << '\n';
        }
        fsup_off.close();

        std::ofstream fsup_srv("registered_serve_supplies.txt");
        for (int i = 0; i < (int)sid_mapper.id2str.size(); ++i) {
            if (i < (int)serve_sid_flag.size() && serve_sid_flag[i]) fsup_srv << sid_mapper.id2str[i] << '\n';
        }
        fsup_srv.close();

        std::ofstream fdid("registered_demands.txt");
        for (const auto &d : did_mapper.id2str) fdid << d << '\n';
        fdid.close();
        std::cerr << "[DEBUG] 已把完整列表写入: registered_offline_supplies.txt, registered_serve_supplies.txt, registered_demands.txt" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[DEBUG] dump_registered_mappings 发生异常: " << e.what() << std::endl;
    }
}

// 基于历史分配计算 per-supply 概率分布；若无历史分配，则在可分配集合上均匀分布
void compute_supply_demand_probabilities() {
    serve_sid_did_prob.assign(serve_sid_did.size(), {});

    for (size_t sid = 0; sid < serve_sid_did.size(); ++sid) {
        if (sid >= serve_sid_flag.size() || !serve_sid_flag[sid]) continue; // 只处理 serve 供给
        const auto &allowed = serve_sid_did[sid];
        if (allowed.empty()) continue; // 无可分配集合则保持空

        double total = 0.0;
        std::unordered_map<int, double> weight;

        // 从 offline 历史分配中收集权重，只保留合法的 demand
        if (sid < offline_sid_did_allocatepv.size()) {
            for (const auto &kv : offline_sid_did_allocatepv[sid]) {
                int did = kv.first;
                if (allowed.find(did) == allowed.end()) continue; // 跳过不合法的边
                int pv = kv.second;
                if (pv <= 0) continue;
                weight[did] += static_cast<double>(pv);
                total += static_cast<double>(pv);
            }
        }

        if (total > 0.0) {
            for (const auto &kv : weight) {
                serve_sid_did_prob[sid][kv.first] = kv.second / total;
            }
        } else {
            double p = 1.0 / static_cast<double>(allowed.size());
            for (int did : allowed) {
                serve_sid_did_prob[sid][did] = p;
            }
        }
    }

}
