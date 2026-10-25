
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <random>
#include "file_reader.hpp"

namespace fs = std::filesystem;

void serve_by_probability() {
    compute_supply_demand_probabilities();

    serve_sid_did_serving_allocatepv.assign(serve_sid_did.size(), {});

    std::vector<int> remaining_demand = did_amount;
    long long total_remaining = 0;
    for (int v : remaining_demand) total_remaining += v;

    std::mt19937 rng(std::random_device{}()); // 可替换为固定种子以便复现

    for (size_t sid = 0; sid < serve_sid_did.size(); ++sid) {
        if (total_remaining <= 0) break; // 需求已全部满足，后续供给直接空着
        if (sid >= serve_sid_flag.size() || !serve_sid_flag[sid]) continue;
        int pv = (sid < serve_sid_pv.size()) ? serve_sid_pv[sid] : 0;
        if (pv <= 0) continue;

        const auto &prob_map = serve_sid_did_prob[sid];
        if (prob_map.empty()) {
            std::cerr << "[Serving] supply '" << sid_mapper.to_str(static_cast<int>(sid))
                      << "' 概率分布为空，跳过" << std::endl;
            continue;
        }

        std::vector<int> dids;
        std::vector<double> weights;
        dids.reserve(prob_map.size());
        weights.reserve(prob_map.size());
        for (const auto &kv : prob_map) {
            dids.push_back(kv.first);
            weights.push_back(kv.second);
        }

        std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
        auto &alloc_map = serve_sid_did_serving_allocatepv[sid];
        for (int i = 0; i < pv; ++i) {
            if (total_remaining <= 0) break; // 需求已满足，剩余 pv 空置
            size_t idx = dist(rng);
            int did = dids[idx];
            if (did < 0 || did >= (int)remaining_demand.size()) continue;
            if (remaining_demand[did] <= 0) continue; // 该 demand 已满，空置本次 pv
            ++alloc_map[did];
            --remaining_demand[did];
            --total_remaining;
        }
    }
}


int main(int argc, char *argv[])
{
    if (argc != 6) {
        std::cerr << "用法: " << argv[0] << " <demand_file> <offline_supply_file> <serve_supply_file> <offline_res_csv> <serving_result_csv>" << std::endl;
        return 1;
    }

    std::string demand_file = argv[1];
    std::string offline_supply_file = argv[2];
    std::string serve_supply_file = argv[3];
    std::string offline_res_csv = argv[4];
    std::string serving_res_csv = argv[5];


    read_offline_supply_indexed(offline_supply_file);
    read_serve_supply_indexed(serve_supply_file);
    read_demand_indexed(demand_file);
    // 输出已注册的 supply/demand 列表，便于调试 result 文件中未注册 ID 的问题
    // dump_registered_mappings();
    read_result_indexed(offline_res_csv);

    // Serving 阶段按概率抽样分配（全量使用 sid_pv）
    serve_by_probability();


    // 确保父目录存在，否则尝试创建
    fs::path out_path(serving_res_csv);
    fs::path parent = out_path.parent_path();
    if (!parent.empty() && !fs::exists(parent)) {
        try {
            fs::create_directories(parent);
        } catch (const std::exception& e) {
            std::cerr << "[Serving分配] 无法创建目录: " << parent << " 错误: " << e.what() << std::endl;
            return 1;
        }
    }

    std::ofstream fout(serving_res_csv, std::ios::trunc);
    if (!fout.is_open() || fout.fail()) {
        std::cerr << "[Serving分配] 无法打开文件写入: " << serving_res_csv << std::endl;
        return 1;
    }

    fout << "sid,demand,allocated_pv\n";
    for (int sid = 0; sid < (int)serve_sid_did_serving_allocatepv.size(); ++sid) {
        if (sid >= (int)serve_sid_flag.size() || !serve_sid_flag[sid]) continue;
        for (const auto& kv : serve_sid_did_serving_allocatepv[sid]) {
            int did = kv.first, pv = kv.second;
            fout << sid_mapper.to_str(sid) << "," << did_mapper.to_str(did) << "," << pv << "\n";
        }
    }
    fout.close();
    if (fout.fail()) {
        std::cerr << "[Serving分配] 写入文件时发生错误: " << serving_res_csv << std::endl;
        return 1;
    }

    std::cout << "\n[Serving分配] 结果已写入: " << serving_res_csv << std::endl;
    return 0;
}
