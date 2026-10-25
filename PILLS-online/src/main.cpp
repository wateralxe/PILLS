
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <algorithm>
#include "file_reader.hpp"

int move_pv(std::vector<int>& unfinished_demands, double threshold)
{ 
    // 找出所有询量需求所需要的 supply
    std::unordered_set<int> unfinished_needed_supply; 
    for (int demand_idx : unfinished_demands) {
        const std::unordered_set<int>& supply_set = did_sid[demand_idx];
        unfinished_needed_supply.insert(supply_set.begin(), supply_set.end());
    }

    // 记录每一个 demand 从被需要的 supply 中分得的 pv
    std::vector<int> demand_allocatepv_from_neededsupply(did_amount.size(), 0); 
    for (int supply_id : unfinished_needed_supply) {
        const auto& allocation_map = sid_did_allocatepv[supply_id]; 
        for (const auto& [demand_id, allocated_pv] : allocation_map) {
            demand_allocatepv_from_neededsupply[demand_id] += allocated_pv; 
        }
    }

    // std::cout << "[DEBUG] move_pv: unfinished_demands: ";
    // for (int did : unfinished_demands) std::cout << did << " ";
    // std::cout << std::endl;

    // 遍历所有已完成的需求，把有余量的 supply 分出来，直到没有可分的或者能完全代替用掉的被需要的 supply
    int max_move_pv = 0;
    for (size_t demand_idx = 0; demand_idx <  did_sid.size(); ++demand_idx) 
    {
        // 跳过询量的 demand 和没有用到 needsupply 的 demand
        if (std::find(unfinished_demands.begin(), unfinished_demands.end(), demand_idx) != unfinished_demands.end()) {
            continue;
        }
        int need_pv = demand_allocatepv_from_neededsupply[demand_idx];
        if (need_pv <= 0) {
            if (need_pv < 0) {
                std::cout << "error: need_pv < 0" << std::endl;
            }
            continue;
        }
        //此时 demand_idx 是那些用到 needsupply 的，所以想要把它们分到那些非 needsupply 中
        std::unordered_set<int> canallocate_sid =  did_sid[demand_idx];


        int moved_pv = 0;
        // 把有余量的 supply 分给当前 demand，记录一共分给了多少
        for (int supply_idx : canallocate_sid) { 
            if (unfinished_needed_supply.find(supply_idx) != unfinished_needed_supply.end()) {
                continue; 
            }
            // 最多可以全部剩余的都分出来，或者是把用的 needsupply 的全部 reallocate 过来
            int tmp_moved_pv = std::min(sid_remainpv[supply_idx], need_pv);
            // if (tmp_moved_pv > 0) {
            //     std::cout << "[DEBUG] 分配: supply " << supply_idx << " -> demand " << demand_idx << " : " << tmp_moved_pv << std::endl;
            // }
            moved_pv += tmp_moved_pv;
            need_pv -= tmp_moved_pv;

            sid_remainpv[supply_idx] -= tmp_moved_pv;
            sid_did_allocatepv[supply_idx][demand_idx] += tmp_moved_pv;
            demand_allocatepv_from_neededsupply[demand_idx] -= tmp_moved_pv; // 好像没意义但还是先写上

            if (need_pv <= 0) {
                if (need_pv < 0) {
                    std::cerr << "Error: need_pv is negative after allocation." << std::endl;
                }
                break;
            }
        }
        max_move_pv += moved_pv;

        int true_move_pv = moved_pv;
        // 把被需要的 supply 分给当前 demand 的 pv 退回去（reverse）
        for (int supply_idx : unfinished_needed_supply) 
        {
            if (sid_did_allocatepv[supply_idx].find(demand_idx) == sid_did_allocatepv[supply_idx].end()) {
                continue; 
            }

            int tmp_moved_pv = std::min(sid_did_allocatepv[supply_idx][demand_idx],true_move_pv) ;
            true_move_pv -= tmp_moved_pv;

            sid_remainpv[supply_idx] += tmp_moved_pv;
            sid_did_allocatepv[supply_idx][demand_idx] -= tmp_moved_pv;

            if (true_move_pv <= 0) {
                if (true_move_pv < 0) {
                    std::cerr << "Error: need_pv is negative after allocation." << std::endl;
                }
                break;
            }
        }
    }
    return max_move_pv; // 返回最大移动的 PV 数量
}
void greedy_allocate_online()
{   
    // 遍历每个 supply
    for (int sid = 0; sid < (int)sid_did.size(); ++sid) {
        // 跳过没有剩余的 supply
        if (sid_remainpv[sid] <= 0) continue;
        // 遍历所有 online demand
        for (int did : online_did) {
            // 只分配 supply 能分配的 online demand
            if (sid_did[sid].find(did) == sid_did[sid].end()) continue;
            int alloc = sid_remainpv[sid];
            if (alloc > 0) {
                sid_did_allocatepv[sid][did] += alloc;
                sid_remainpv[sid] -= alloc;
                // greedy: 该 supply 全部分给第一个能分的 online demand
                break;
            }
        }
    }
}
int IncLS_indexed(const std::string&offline_res_csv,const std::string& online_res_csv) 
{
    int max_move_pv = move_pv(online_did, 1.0);
    greedy_allocate_online();
    return max_move_pv;
    
}


int main(int argc, char *argv[])
{
    if (argc != 7) {
        std::cerr << "用法: " << argv[0] << " <demand_file> <supply_file> <offline_res_csv> <online_demand_file> <online_result_csv> <cutoff_time_sec>" << std::endl;
        return 1;
    }

    std::string demand_file = argv[1];
    std::string supply_file = argv[2];
    std::string offline_res_csv = argv[3];
    std::string online_demand_file = argv[4];
    std::string online_res_csv = argv[5];
    int cutoff_time = std::stoi(argv[6]);


    read_supply_indexed(supply_file);
    read_demand_indexed(demand_file);
    read_online_demand_indexed(online_demand_file);
    read_result_indexed(offline_res_csv);

    auto start_time = std::chrono::high_resolution_clock::now();
    int k =0;
    while (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time).count() < cutoff_time) 
    {
        std::cout << "第" << k++ << "次分配" << std::endl;
        int max_move_pv = IncLS_indexed(offline_res_csv,online_res_csv);
        if(max_move_pv <= 0)break;
    }
    // 输出完整分配结果csv（原有分配+online分配）
    std::ofstream fout(online_res_csv);
    fout << "sid,demand,allocated_pv\n";
    for (int sid = 0; sid < (int)sid_did_allocatepv.size(); ++sid) {
        for (const auto& kv : sid_did_allocatepv[sid]) {
            int did = kv.first, pv = kv.second;
            fout << sid_mapper.to_str(sid) << "," << did_mapper.to_str(did) << "," << pv << "\n";
        }
    }
    fout.close();
    std::cout << "\n[Online分配] 结果已写入: " << online_res_csv << std::endl;
    return 0;
}
