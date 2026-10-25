#include "model.h"
#include "solver.h"
#include "mode.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/stat.h>
#include <cstdlib>
#include <sstream>
#include <limits>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <regex>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

/**
 * @brief Data Format Description
 * 1. integer dataset:  user id | supply id | cast times | demand_list (split by ,)
 * 2. demand dataset: demand id | demand amount
 * 3. var's name change
 */

namespace solver
{
    void split(std::string &str, std::vector<std::string> &vec, char x)
    {
        vec.clear();
        int start = 0, end = 0;
        for (int i = 0; i < str.length(); i++)
        {
            if (str[i] == x)
            {
                end = i;
                vec.push_back(str.substr(start, end - start));
                start = i + 1;
            }
        }
        end = str.length();
        if (end - start > 0)
        {
            vec.push_back(str.substr(start, end - start));
        }
    }

    sls_model::sls_model(std::string integer_set, std::string demand_set, std::string heat_set) : format_supply_path(integer_set), demand_path(demand_set), heat_path(heat_set) {}

    void sls_model::read_demand_data()
    {
        demand_id_amount.clear();
        std::ifstream demand_file(demand_path);
        std::string record;
        while (getline(demand_file, record))
        {
            std::vector<std::string> strs;
            split(record, strs, '`');
            demand_id_amount[strs[0]] = std::stoi(strs[1]);
            // demand_id_amount[strs[0]] = std::stoi(strs[1]) * 1.0 / 50000; // 5w original
        }
        std::cout << "demand data size: " << demand_id_amount.size() << std::endl;
        demand_file.close();
    }

    void sls_model::read_heat_data()
    {
        user_supply_heat.clear(); // 清空之前的热度数据
        std::ifstream heat_file(heat_path); // 假设 heat_path 是存储热度数据的文件路径
        if (!heat_file.is_open()) {
            std::cerr << "无法打开热度数据文件: " << heat_path << std::endl;
            return;
        }

        std::string record;
        while (std::getline(heat_file, record)) {
            std::vector<std::string> strs;
            split(record, strs, '`'); // 使用相同的分隔符解析每一行

            if (strs.size() >= 3) { // 确保每一行有足够的字段
                std::string user_id = strs[0];
                std::string supply_id = strs[1];
                double heat_coefficient = std::stod(strs[2]);

                // 将热度系数存入 user_supply_heat 数据结构
                user_supply_heat[user_id][supply_id] = heat_coefficient;
            } else {
                std::cerr << "警告：跳过格式不正确的行 - " << record << std::endl;
            }
        }

        heat_file.close();
    }


    void sls_model::read_integer_data()
    {
        user_supply_cast_times.clear();
        user_supply_demand_set.clear();
        supply_user_demand_set.clear();
        supply_demand_set.clear();
        supply_with_more_demand.clear();
        std::ifstream integer_file(format_supply_path);
        std::string record;
        // user`supply`demand1,demand2
        while (getline(integer_file, record))
        {
            // only read ?%
            // if (rand() % 10000 < 5000)
            // {
            //     continue;
            // }
            std::vector<std::string> strs;
            split(record, strs, '`');
            user_supply_cast_times[strs[0]][strs[1]] = std::stoi(strs[2]);
            std::vector<std::string> demand_list;
            split(strs[3], demand_list, ',');
            for (auto demand_id : demand_list)
            {
                if (user_supply_demand_set[strs[0]][strs[1]].count(demand_id) == 0)
                {
                    user_supply_demand_set[strs[0]][strs[1]].insert(demand_id);
                }
                if (supply_user_demand_set[strs[1]][strs[0]].count(demand_id) == 0)
                {
                    supply_user_demand_set[strs[1]][strs[0]].insert(demand_id);
                }
                if (supply_demand_set[strs[1]].count(demand_id) == 0)
                {
                    supply_demand_set[strs[1]].insert(demand_id);
                }
            }
        }
        for (auto ele : supply_demand_set)
        {
            if (ele.second.size() >= 2)
            {
                supply_with_more_demand.insert(ele.first);
            }
        }
        integer_file.close();
    }

    /**
     * All ids or strs are used with converted ids
     */
    void sls_model::model_problem(std::string lp_file, std::string sol_file, int nia_num, std::string result_file, ModelMode mode,int time_limit)
    {
        std::cout << "user num | supply num | demand num\n";
        std::cout << user_supply_cast_times.size() << " | " << supply_demand_set.size() << " | " << demand_id_amount.size() << std::endl;
        
        // rand();//nor for original
        bool enable_given_query = false;
        bool enable_rand_query = false;
        bool enable_max_query = false;
        if (mode == GivenQueryObj)
        {
            enable_given_query = true;
            std::cout << "需要修改给定查询的需求节点，请在代码中修改给定查询的需求节点。" << std::endl;
        }
        else
        {
            if (mode == RandQueryObj)
            {
                enable_rand_query = true;
            }
            else if (mode == MaxQueryObj)
            {
                enable_max_query = true;
            }
        }
        std::unordered_set<std::string> query_strs;
        if(enable_given_query)
        {
            std::vector<std::string> given_query_demands = {""}; 
            for (const auto& given_query_demand : given_query_demands)
            {
                for (auto it = demand_id_convert.begin(); it != demand_id_convert.end(); it++)
                {
                    if (given_query_demand == it->first)
                    {
                        query_strs.insert(it->second);
                        break;
                    }
                }
            }
            std::cout << "Using given query selection." << std::endl;
            std::cout << "Selected query strings: ";
            for (const auto& query_str : query_strs)
            {
                std::cout << query_str << " ";
            }
            std::cout << std::endl;
        }
        else 
        {
            // Use random query selection
            if (enable_rand_query)
            {
                // rand();
                int query_id = rand() % demand_id_convert.size();
                // Random a query demand node
                int cnt = 0;
                for (auto it = demand_id_convert.begin(); it != demand_id_convert.end(); it++)
                {
                    if (cnt == query_id)
                    {
                        // query_str = it->second;
                        query_strs.insert(it->second);
                        break;
                    }
                    cnt++;
                }
                std::cout << "Using random query selection." << std::endl;
                std::cout << "Selected query strings: ";
                for (const auto& query_str : query_strs)
                {
                    std::cout << query_str << " ";
                }
                std::cout << std::endl;
            }
            else if (enable_max_query)
            {
                // Demand with Maximum var count
                rand();
                std::unordered_map<std::string, int> demand_var_count;
                for (auto ele1 : user_supply_demand_set)
                {
                    auto user_id = ele1.first;
                    for (auto ele2 : ele1.second)
                    {
                        auto supply_id = ele2.first;
                        for (auto demand_id : ele2.second)
                        {
                            demand_var_count[demand_id]++;
                        }
                    }
                }
                int max_count = 0;
                std::vector<int> demand_nums;
                for (auto ele : demand_var_count)
                {
                    demand_nums.push_back(ele.second);
                    if (ele.second > max_count)
                    {
                        max_count = ele.second;
                        // query_str = ele.first;
                        query_strs.insert(ele.first);
                    }
                }
                std::cout << demand_nums.size() << std::endl;
                sort(demand_nums.begin(), demand_nums.end());
                reverse(demand_nums.begin(), demand_nums.end());
                int top_n_count = demand_nums[10];
                std::cout << top_n_count << std::endl;
                for (auto ele : demand_var_count)
                {
                    if (ele.second == top_n_count)
                    {
                        // query_str = ele.first;
                        query_strs.insert(ele.first);
                    }
                }
                std::cout << "Using variable count-based query selection." << std::endl;
                std::cout << "Selected query strings: ";
                for (const auto& query_str : query_strs)
                {
                    std::cout << query_str << " ";
                }
                std::cout << std::endl;
            }
            else
            {
                std::cout << "No query selection mode specified, using empty query." << std::endl;
            }
        }

        var_name_index.clear();
        demand_vars.clear();
        // register vars
        for (auto ele1 : user_supply_demand_set)
        {
            auto user_id = ele1.first;
            for (auto ele2 : ele1.second
            )
            {
                auto supply_id = ele2.first;
                for (auto demand_id : ele2.second)
                {
                    std::string var_name = "x_" + user_id_convert[user_id] + "_" + supply_id_convert[supply_id] + "_" + demand_id_convert[demand_id];
                    double var_coeff = 0.0;

                    // 如果是查询字符串对应的需求节点，则系数为 -1.0
                    if (enable_given_query && query_strs.count(demand_id_convert[demand_id]))
                    {
                        var_coeff = -1.0;
                    }
                    else if (enable_rand_query && query_strs.count(demand_id_convert[demand_id]))
                    {
                        var_coeff = -1.0;
                    }
                    else if (enable_max_query && query_strs.count(demand_id_convert[demand_id]))
                    {
                        var_coeff = -1.0;
                    }
                    else  // 如果不是查询字符串对应的需求节点，则系数为 0
                    {
                        var_coeff = 0.0;
                    }

                    int var_index = m_sls_solver.register_var(var_name, var_coeff, true, true, 0, user_supply_cast_times[user_id][supply_id]);
                    var_name_index[var_name] = var_index;
                    demand_vars[demand_id].push_back(var_index);
                }
                // 从 user_supply_heat 中获取对应的热度系数
                double var_coeff = 0.0;
                if (mode == HeatObj) {
                    if (user_supply_heat.find(user_id) != user_supply_heat.end() && user_supply_heat[user_id].find(supply_id) != user_supply_heat[user_id].end()) {
                        var_coeff = user_supply_heat[user_id][supply_id]; // 获取热度值
                    } else {
                        std::cerr << "警告：未找到用户 " << user_id << " 和供应方 " << supply_id << " 的热度值，使用默认值 0.0\n";
                    }
                }
                else if (mode == ZeroObj) {
                    var_coeff = 0.0;
                }
                std::string supply_var_name = "x_" + user_id_convert[user_id] + "_" + supply_id_convert[supply_id];
                int supply_var_index = m_sls_solver.register_var(supply_var_name, var_coeff, true, true, 0, user_supply_cast_times[user_id][supply_id]);
                var_name_index[supply_var_name] = supply_var_index;

            }
        }
        std::unordered_map<std::string, int> slack_vars;
        double BIG_COEFFICIENT = 31708040;
        for (auto ele : demand_vars) {
            auto demand_id = ele.first;
            std::string slack_var_name = "p_" + demand_id_convert[demand_id];
            int slack_var_index = m_sls_solver.register_var(slack_var_name, BIG_COEFFICIENT, true, true, 0, INFINITY);
            slack_vars[demand_id] = slack_var_index;
        }
        

        std::cout << "register vars done\n";


        // 2.1 user-supply: sum of vars <= cast count；sum of vars = x_user_supply
        for (auto ele1 : user_supply_demand_set)
        {
            auto user_id = ele1.first;
            for (auto ele2 : ele1.second)
            {
                auto supply_id = ele2.first;
                int supply_var_index = var_name_index["x_" + user_id_convert[user_id] + "_" + supply_id_convert[supply_id]];

                monomial_vector m_monomials;

                // 第一条约束：需求变量的总和等于新的变量 x_user_supply
                for (auto demand_id : ele2.second)
                {
                    std::string var_name = "x_" + user_id_convert[user_id] + "_" + supply_id_convert[supply_id] + "_" + demand_id_convert[demand_id];
                    int_table vars;
                    vars.insert(var_name_index[var_name]);
                    m_monomials.push_back(monomial(1.0, vars));
                }
                int_table supply_vars;
                supply_vars.insert(supply_var_index);
                m_monomials.push_back(monomial(-1.0, supply_vars)); // 添加新的变量到约束中
                polynomial poly1(m_monomials);
                m_sls_solver.register_constraint(poly1, constraint_kind::CAST, "user_supply_sum_" + user_id + "_" + supply_id, true, true, 0, 0);

                // 第二条约束：新的变量 x_user_supply <= cast_count
                // monomial_vector m_supply_monomials;
                // int_table single_supply_var;
                // single_supply_var.insert(supply_var_index);
                // m_supply_monomials.push_back(monomial(1.0, single_supply_var));
                // polynomial poly2(m_supply_monomials);
                // m_sls_solver.register_constraint(poly2, constraint_kind::CAST, "user_supply_bound_" + user_id + "_" + supply_id, false, true, 0, user_supply_cast_times[user_id][supply_id]);

                monomial_vector m_supply_monomials;
                for (auto demand_id : ele2.second)
                {
                    std::string var_name = "x_" + user_id_convert[user_id] + "_" + supply_id_convert[supply_id] + "_" + demand_id_convert[demand_id];
                    int_table vars;
                    vars.insert(var_name_index[var_name]);
                    m_supply_monomials.push_back(monomial(1.0, vars));
                }
                polynomial poly2(m_supply_monomials);
                m_sls_solver.register_constraint(poly2, constraint_kind::CAST, "user_supply_" + user_id + "_" + supply_id, false, true, 0, user_supply_cast_times[user_id][supply_id]);

                }
        }
        std::cout << "2.1 supply done\n";

        // 2.2 demand: sum of vars + slack var >= demand need
        for (auto ele : demand_vars) {
            monomial_vector m_monomials;
            auto demand_id = ele.first;
            int demand_amount = demand_id_amount[demand_id];
        
            // 添加原有变量
            for (auto m_var : ele.second) {
                int_table vars;
                vars.insert(m_var);
                m_monomials.push_back(monomial(1.0, vars));
            }
        
            // 添加辅助变量
            int_table slack_vars_table;
            slack_vars_table.insert(slack_vars[demand_id]);
            m_monomials.push_back(monomial(1.0, slack_vars_table));
        
            polynomial poly(m_monomials);
            m_sls_solver.register_constraint(poly, constraint_kind::DEMAND, "demand_" + demand_id, true, false, demand_amount, 0);
        }
        
        std::cout << "2.2 demand done\n";


        std::cout << "start solve\n";
        m_sls_solver.write_lp_file(lp_file);
        std::cout << "write lp file done\n";
        // 使用 RAP 算法求解
        solve_with_rap(lp_file, sol_file, mode, std::chrono::steady_clock::now(), time_limit);
    }

    void sls_model::generate_id_convert()
    {
        user_id_convert.clear();
        supply_id_convert.clear();
        demand_id_convert.clear();
        int idx;
        for (auto ele1 : user_supply_demand_set)
        {
            auto user_id = ele1.first;
            if (user_id_convert.count(user_id) == 0)
            {
                idx = user_id_convert.size();
                user_id_convert[user_id] = std::to_string(idx);
            }
            for (auto ele2 : ele1.second)
            {
                auto supply_id = ele2.first;
                if (supply_id_convert.count(supply_id) == 0)
                {
                    idx = supply_id_convert.size();
                    supply_id_convert[supply_id] = std::to_string(idx);
                }
                for (auto demand_id : ele2.second)
                {
                    if (demand_id_convert.count(demand_id) == 0)
                    {
                        idx = demand_id_convert.size();
                        demand_id_convert[demand_id] = std::to_string(idx);
                    }
                }
            }
        }
    }

    void sls_model::solve_problem(std::string lp_file,std::string sol_path, int nia_num, std::string result_file, ModelMode mode,int time_limit)
    {
        read_demand_data();
        read_integer_data();
        read_heat_data();
        generate_id_convert();
        model_problem(lp_file,sol_path, nia_num, result_file,mode,time_limit);
    }

    /**
     * @brief Demo for write lp file
     *
     * Minimize -3 x1 - 5 x2 - 2 x3 + x4
     *
     * x1 + 2 x2 * x3 + x2 <= 15
     * 2 x1 + 4 x2 + 5 x3 + x4 >= 10
     * x1 - 3 x2 + 4 x3 + x4 = 5
     * x2 + [3 x2 * x3  - 4 x1 * x2 ] <= 10
     *
     * x1 in [0, 10]
     * x2 in [0, 5]
     * x3 <= 3
     * x4 >= 0s
     */
    void sls_model::demo_solve(std::string str)
    {
        m_sls_solver.register_var("x1", -3, true, true, 0, 10);
        m_sls_solver.register_var("x2", -5, true, true, 0, 5);
        m_sls_solver.register_var("x3", -2, false, true, 0, 3);
        m_sls_solver.register_var("x4", 1, true, false, 0, 0);

        long_double_vector coeff_vec = {1.0, 2.0, 1.0};
        int_table_vector mono_vars = {{0}, {1, 2}, {1}};
        polynomial poly1(coeff_vec, mono_vars);
        m_sls_solver.register_constraint(poly1, constraint_kind::OTHER, "c1", false, true, 0, 15);
        coeff_vec = {2.0, 4.0, 5.0, 1.0};
        mono_vars = {{0}, {1}, {2}, {3}};
        polynomial poly2(coeff_vec, mono_vars);
        m_sls_solver.register_constraint(poly2, constraint_kind::OTHER, "c2", true, false, 10, 0);
        coeff_vec = {1.0, -3.0, 4.0, 1.0};
        mono_vars = {{0}, {1}, {2}, {3}};
        polynomial poly3(coeff_vec, mono_vars);
        m_sls_solver.register_constraint(poly3, constraint_kind::OTHER, "c3", true, true, 5, 5);
        coeff_vec = {1.0, 3.0, -4.0};
        mono_vars = {{1}, {1, 2}, {0, 1}};
        polynomial poly4(coeff_vec, mono_vars);
        m_sls_solver.register_constraint(poly4, constraint_kind::OTHER, "c4", false, true, 0, 10);

        m_sls_solver.set_cutoff(0.5);
        // m_sls_solver.set_stepoff(1000);
        m_sls_solver.solve();
        m_sls_solver.display_results(std::cout);
    }

    /**
     * @brief Demo for write lp file
     *
     * Minimize -3 x1 - 5 x2 - 2 x3 + x4
     *
     * x1 + 2 x2 * x3 + x2 <= 15
     * 2 x1 + 4 x2 + 5 x3 + x4 >= 10
     * x1 - 3 x2 + 4 x3 + x4 = 5
     * x2 + [3 x2 * x3  - 4 x1 * x2 ] <= 10
     *
     * x1 in [0, 10]
     * x2 in [0, 5]
     * x3 <= 3
     * x4 >= 0
     */
    void sls_model::demo_write_lp(std::string str)
    {
        m_sls_solver.register_var("x1", -3, true, true, 0, 10);
        m_sls_solver.register_var("x2", -5, true, true, 0, 5);
        m_sls_solver.register_var("x3", -2, false, true, 0, 3);
        m_sls_solver.register_var("x4", 1, true, false, 0, 0);

        long_double_vector coeff_vec = {1.0, 2.0, 1.0};
        int_table_vector mono_vars = {{0}, {1, 2}, {1}};
        polynomial poly1(coeff_vec, mono_vars);
        m_sls_solver.register_constraint(poly1, constraint_kind::OTHER, "c1", false, true, 0, 15);
        coeff_vec = {2.0, 4.0, 5.0, 1.0};
        mono_vars = {{0}, {1}, {2}, {3}};
        polynomial poly2(coeff_vec, mono_vars);
        m_sls_solver.register_constraint(poly2, constraint_kind::OTHER, "c2", true, false, 10, 0);
        coeff_vec = {1.0, -3.0, 4.0, 1.0};
        mono_vars = {{0}, {1}, {2}, {3}};
        polynomial poly3(coeff_vec, mono_vars);
        m_sls_solver.register_constraint(poly3, constraint_kind::OTHER, "c3", true, true, 5, 5);
        coeff_vec = {1.0, 3.0, -4.0};
        mono_vars = {{1}, {1, 2}, {0, 1}};
        polynomial poly4(coeff_vec, mono_vars);
        m_sls_solver.register_constraint(poly4, constraint_kind::OTHER, "c4", false, true, 0, 10);

        m_sls_solver.write_lp_file(str);
    }


    static inline std::string trim_str(const std::string &s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return std::string();
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    }
    static inline std::string to_lower_str(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
        return s;
    }
    static inline std::vector<double> matvec_prod(const std::vector<std::vector<double>> &A, const std::vector<double> &x) {
        int m = (int)A.size();
        std::vector<double> out(m, 0.0);
        for (int i = 0; i < m; ++i) {
            double s = 0.0;
            const std::vector<double> &row = A[i];
            int n = (int)row.size();
            for (int j = 0; j < n && j < (int)x.size(); ++j) s += row[j] * x[j];
            out[i] = s;
        }
        return out;
    }
    static inline std::vector<double> matT_vec_prod(const std::vector<std::vector<double>> &A, const std::vector<double> &y, int ncols) {
        std::vector<double> out(ncols, 0.0);
        int m = (int)A.size();
        for (int i = 0; i < m; ++i) {
            double yi = y[i];
            const std::vector<double> &row = A[i];
            int n = (int)row.size();
            for (int j = 0; j < n && j < ncols; ++j) out[j] += row[j] * yi;
        }
        return out;
    }
    static inline void expand_rows_to_n(std::vector<std::vector<double>> &A, int n) {
        for (auto &row : A) if ((int)row.size() < n) row.resize(n, 0.0);
    }

    void sls_model::write_solution(const std::string &filename, const std::vector<std::string> &var_list, const std::vector<double> &x) 
    {
        std::ofstream ofs(filename);
        if (!ofs.is_open()) return;
        for (size_t i = 0; i < var_list.size() && i < x.size(); ++i) 
        {
            int xi_rounded = static_cast<int>(std::round(x[i]));
            ofs << var_list[i] << " " << xi_rounded << "\n";
        }
        ofs.close();
    }
    // void sls_model::solve_with_rap(const std::string& lp_file_path, const std::string& sol_file, ModelMode mode, const std::chrono::steady_clock::time_point& start_time, int time_limit)
    // {
    //     // 1) 初始化参数和超参数
    //     // const int max_iter = 20000000;
    //     const double alpha0 = 0.00001;        // 步长基准（alpha_t = alpha0 / sqrt(t+1)）
    //     const double tol = 1e-6;           // 收敛容差（约束残差）
    //     const double inf_bound = 1e8;      // 用于代替 "inf" 的大值


    //     std::ifstream ifs(lp_file_path);
    //     if (!ifs.is_open()) {
    //         std::cerr << "[RAP] 无法打开LP文件: " << lp_file_path << std::endl;
    //         return;
    //     }
    //     std::ostringstream oss;
    //     oss << ifs.rdbuf();
    //     std::string text = oss.str();
    //     ifs.close();

    //     std::string ltext = to_lower_str(text);
    //     auto pos_of = [&](const std::string &kw)->int {
    //         auto p = ltext.find(to_lower_str(kw));
    //         return p == std::string::npos ? -1 : (int)p;
    //     };
    //     int pMin = pos_of("minimize");
    //     int pSub = pos_of("subject to");
    //     int pBounds = pos_of("bounds");
    //     int pGen = pos_of("generals");
    //     int pEnd = pos_of("end");

    //     auto section = [&](int a, int b)->std::string {
    //         if (a < 0) return "";
    //         if (b < 0) b = (int)text.size();
    //         if (b <= a) return "";
    //         return text.substr(a, b-a);
    //     };

    //     std::string minimize_sec = section(pMin, (pSub>0? pSub : (pBounds>0? pBounds : (pGen>0? pGen : pEnd))));
    //     std::string subject_sec  = section(pSub, (pBounds>0? pBounds : (pGen>0? pGen : pEnd)));
    //     std::string bounds_sec   = section(pBounds, (pGen>0? pGen : pEnd));
    //     std::string generals_sec = section(pGen, pEnd);

    //     // 2) 读取 LP 文件并获取变量、目标系数、二次系数、上下界等（按项注册变量）
    //     std::unordered_map<std::string,int> var_index;
    //     std::vector<std::string> var_list;
    //     std::vector<double> c;
    //     std::vector<double> w;
    //     std::vector<double> lb;
    //     std::vector<double> ub;

    //     auto is_keyword = [&](const std::string &s)->bool {
    //         std::string sl = to_lower_str(s);
    //         if (sl=="minimize" || sl=="subject" || sl=="to" || sl=="bounds" || sl=="generals" || sl=="end") return true;
    //         if (sl=="obj" || sl=="obj:") return true;
    //         return false;
    //     };

    //     auto ensure_var = [&](const std::string &name)->int {
    //         if (name.empty()) return -1;
    //         std::string nl = name;
    //         if (is_keyword(nl)) return -1;
    //         std::string nl_low = to_lower_str(nl);
    //         if (nl_low == "inf") return -1;
    //         auto it = var_index.find(nl);
    //         if (it != var_index.end()) return it->second;
    //         int id = (int)var_list.size();
    //         var_index[nl] = id;
    //         var_list.push_back(nl);
    //         c.push_back(0.0);
    //         w.push_back(0.0);
    //         lb.push_back(0.0);
    //         ub.push_back(inf_bound);
    //         return id;
    //     };

    //     if (!minimize_sec.empty()) {
    //         size_t lbpos = minimize_sec.find('[');
    //         size_t rbpos = std::string::npos;
    //         if (lbpos != std::string::npos) rbpos = minimize_sec.find(']', lbpos+1);
    //         if (lbpos != std::string::npos && rbpos != std::string::npos) {
    //             std::string quad_body = minimize_sec.substr(lbpos+1, rbpos - lbpos - 1);
    //             std::regex quad_term_re(R"(([+-]?\s*(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)?\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:\^?2)?)");
    //             auto it = std::sregex_iterator(quad_body.begin(), quad_body.end(), quad_term_re);
    //             auto end = std::sregex_iterator();
    //             for (; it != end; ++it) {
    //                 std::smatch m = *it;
    //                 std::string coef_s = m.size()>1 ? m[1].str() : "";
    //                 std::string varname = m.size()>2 ? m[2].str() : "";
    //                 if (varname.empty()) continue;
    //                 int id = ensure_var(varname);
    //                 if (id < 0) continue;
    //                 double coef = 1.0;
    //                 if (!coef_s.empty()) {
    //                     coef_s.erase(remove_if(coef_s.begin(), coef_s.end(), ::isspace), coef_s.end());
    //                     try { coef = std::stod(coef_s); } catch(...) { continue; }
    //                 }
    //                 w[id] += coef;
    //             }
    //         }
    //         std::string linear_part = minimize_sec;
    //         if (lbpos != std::string::npos && rbpos != std::string::npos) {
    //             linear_part.erase(lbpos, rbpos - lbpos + 1);
    //         }
    //         std::regex lin_term_re(R"(([+-]?\s*(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)?\s*([A-Za-z_][A-Za-z0-9_]*))");
    //         {
    //             auto it = std::sregex_iterator(linear_part.begin(), linear_part.end(), lin_term_re);
    //             auto end = std::sregex_iterator();
    //             for (; it != end; ++it) {
    //                 std::smatch m = *it;
    //                 std::string coef_s = m.size()>1 ? m[1].str() : "";
    //                 std::string varname = m.size()>2 ? m[2].str() : "";
    //                 if (varname.empty()) continue;
    //                 int id = ensure_var(varname);
    //                 if (id < 0) continue;
    //                 double coef = 1.0;
    //                 if (!coef_s.empty()) {
    //                     coef_s.erase(remove_if(coef_s.begin(), coef_s.end(), ::isspace), coef_s.end());
    //                     try { coef = std::stod(coef_s); } catch(...) { continue; }
    //                 }
    //                 c[id] += coef;
    //             }
    //         }
    //     }

    //     if (!bounds_sec.empty()) {
    //         std::stringstream ss(bounds_sec);
    //         std::string line;
    //         std::regex bound_re(R"(([-+eE0-9\.infINF]+)\s*<=\s*([A-Za-z_][A-Za-z0-9_]*)\s*<=\s*([-\+eE0-9\.infINF]+))", std::regex::icase);
    //         while (std::getline(ss, line)) {
    //             line = trim_str(line);
    //             if (line.empty()) continue;
    //             std::smatch m;
    //             if (std::regex_search(line, m, bound_re)) {
    //                 std::string left = m[1].str();
    //                 std::string varname = m[2].str();
    //                 std::string right = m[3].str();
    //                 int id = ensure_var(varname);
    //                 if (id < 0) continue;
    //                 auto parseb = [&](const std::string &s)->double {
    //                     std::string t = s;
    //                     t.erase(remove_if(t.begin(), t.end(), ::isspace), t.end());
    //                     std::string tl = to_lower_str(t);
    //                     if (tl.find("inf") != std::string::npos) return inf_bound;
    //                     try { return std::stod(t); } catch(...) { return 0.0; }
    //                 };
    //                 lb[id] = parseb(left);
    //                 ub[id] = parseb(right);
    //             }
    //         }
    //     }

    //     std::vector<std::vector<double>> A_eq;
    //     std::vector<double> b_eq;
    //     std::vector<std::vector<double>> A_ineq;
    //     std::vector<double> b_ineq;

    //     if (!subject_sec.empty()) {
    //         std::stringstream ss(subject_sec);
    //         std::string raw;
    //         std::regex rel_re(R"(<=|>=|=)");
    //         std::regex term_re(R"(([+-]?\s*(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)?\s*([A-Za-z_][A-Za-z0-9_]*))");
    //         while (std::getline(ss, raw)) {
    //             std::string line = trim_str(raw);
    //             if (line.empty()) continue;
    //             size_t colpos = line.find(':');
    //             if (colpos != std::string::npos) line = trim_str(line.substr(colpos+1));
    //             std::smatch rm;
    //             if (!std::regex_search(line, rm, rel_re)) continue;
    //             std::string rel = rm.str(0);
    //             size_t rpos = line.find(rel);
    //             std::string lhs = trim_str(line.substr(0, rpos));
    //             std::string rhs = trim_str(line.substr(rpos + rel.size()));
    //             double rhs_val = 0.0;
    //             try { rhs_val = std::stod(rhs); } catch(...) { continue; }

    //             std::vector<double> coeffs(var_list.size(), 0.0);
    //             auto it = std::sregex_iterator(lhs.begin(), lhs.end(), term_re);
    //             auto end = std::sregex_iterator();
    //             for (; it != end; ++it) {
    //                 std::smatch m = *it;
    //                 std::string coef_s = m.size()>1 ? m[1].str() : "";
    //                 std::string varname = m.size()>2 ? m[2].str() : "";
    //                 if (varname.empty()) continue;
    //                 double coef = 1.0;
    //                 if (!coef_s.empty()) {
    //                     coef_s.erase(remove_if(coef_s.begin(), coef_s.end(), ::isspace), coef_s.end());
    //                     try { coef = std::stod(coef_s); } catch(...) { coef = 1.0; }
    //                 }
    //                 int id = ensure_var(varname);
    //                 if (id < 0) continue;
    //                 if ((int)coeffs.size() < (int)var_list.size()) coeffs.resize(var_list.size(), 0.0);
    //                 coeffs[id] += coef;
    //             }

    //             if (rel == "=") {
    //                 A_eq.push_back(coeffs);
    //                 b_eq.push_back(rhs_val);
    //             } else if (rel == ">=") {
    //                 A_ineq.push_back(coeffs);
    //                 b_ineq.push_back(rhs_val);
    //             } else {
    //                 for (int j=0;j<(int)coeffs.size();++j) coeffs[j] = -coeffs[j];
    //                 A_ineq.push_back(coeffs);
    //                 b_ineq.push_back(-rhs_val);
    //             }
    //         }
    //     }

    //     int n = (int)var_list.size();
    //     expand_rows_to_n(A_eq, n);
    //     expand_rows_to_n(A_ineq, n);

    //     // std::cout << "[RAP] 解析得到 " << n << " 个变量：" << std::endl;
    //     // for (int i=0;i<n;++i) {
    //     //     std::cout << "  " << var_list[i] << "  c=" << c[i] << " w=" << w[i] << " lb=" << lb[i] << " ub=" << ub[i] << std::endl;
    //     // }

    //     int m_eq = (int)A_eq.size();
    //     int m_ineq = (int)A_ineq.size();

    //     // 3) 初始化对偶变量（mu, lam）
    //     std::vector<double> x(n, 0.0);
    //     for (int j=0;j<n;++j) {
    //         if (std::isfinite(lb[j]) && lb[j] > -inf_bound/2) x[j] = lb[j];
    //         else x[j] = 0.0;
    //     }
    //     std::vector<double> mu(m_ineq, 0.0);
    //     std::vector<double> lam(m_eq, 0.0);

        
    //     std::cout << "[RAP] 开始对偶子梯度迭代（时间限制："<<time_limit<<"秒）" << std::endl;
    //     for (int t = 0; ; ++t) 
    //     {
    //         // 4) 通过 KKT 条件（stationarity）用对偶变量求原始变量 x
    //         double alpha = alpha0 / std::sqrt((double)(t + 1));

    //         std::vector<double> r = c;
    //         if (m_eq > 0) {
    //             std::vector<double> tmp = matT_vec_prod(A_eq, lam, n);
    //             for (int j = 0; j < n; ++j) r[j] += tmp[j];
    //         }
    //         if (m_ineq > 0) {
    //             std::vector<double> tmp = matT_vec_prod(A_ineq, mu, n);
    //             for (int j = 0; j < n; ++j) r[j] += tmp[j];
    //         }

    //         if (t == 0) {
    //             int show = std::min(20, n);
    //             // std::cout << "[RAP] 初始 reduced costs (前 " << show << " 个变量):" << std::endl;
    //             // for (int j = 0; j < show; ++j) {
    //             //     std::cout << "   " << var_list[j] << " r=" << r[j] << " w=" << w[j] << " lb=" << lb[j] << " ub=" << ub[j] << std::endl;
    //             // }
    //         }

    //         std::vector<double> x_prev = x;
    //         for (int j=0;j<n;++j) {
    //             if (w[j] > 0.0) {
    //                 double zj = - r[j] / w[j];
    //                 if (zj < lb[j]) zj = lb[j];
    //                 if (zj > ub[j]) zj = ub[j];
    //                 x[j] = zj;
    //             } else {
    //                 if (r[j] > 0.0) x[j] = lb[j];
    //                 else if (r[j] < 0.0) x[j] = ub[j];
    //                 else x[j] = x_prev[j];
    //             }
    //         }

    //         // 5) 通过原始变量 x 计算对偶变量的梯度（残差）
    //         std::vector<double> s_eq(m_eq, 0.0), s_ineq(m_ineq, 0.0);
    //         if (m_eq>0) {
    //             s_eq = matvec_prod(A_eq, x);
    //             for (int i=0;i<m_eq;++i) s_eq[i] -= b_eq[i];
    //         }
    //         if (m_ineq>0) {
    //             s_ineq = matvec_prod(A_ineq, x);
    //             for (int i=0;i<m_ineq;++i) s_ineq[i] -= b_ineq[i];
    //         }

    //         // 6) 使用子梯度（残差）更新对偶变量（对 μ 做非负投影）
    //         for (int i=0;i<m_ineq;++i) {
    //             mu[i] += alpha * s_ineq[i];
    //             if (mu[i] < 0.0) mu[i] = 0.0;
    //         }
    //         for (int i=0;i<m_eq;++i) lam[i] += alpha * s_eq[i];

    //         double max_eq_viol = 0.0;
    //         for (int i=0;i<m_eq;++i) max_eq_viol = std::max(max_eq_viol, std::fabs(s_eq[i]));
    //         double max_ineq_viol = 0.0;
    //         for (int i=0;i<m_ineq;++i) {
    //             double Ax = 0.0;
    //             for (int j=0;j<n;++j) Ax += A_ineq[i][j] * x[j];
    //             double viol = std::max(0.0, b_ineq[i] - Ax);
    //             if (viol > max_ineq_viol) max_ineq_viol = viol;
    //         }

    //         // 输出当前效果
    //         if (t % 1000 == 0) {
    //             double quad = 0.0;
    //             for (int j = 0; j < n; ++j) quad += w[j] * x[j] * x[j];
    //             double obj = 0.5 * quad;
    //             for (int j = 0; j < n; ++j) obj += c[j] * x[j];
    //             std::cout << "[RAP] iter=" << t << " obj=" << obj
    //                       << " max_eq_viol=" << max_eq_viol
    //                       << " max_ineq_viol=" << max_ineq_viol << std::endl;

    //             // 检查时间限制
    //             auto current_time = std::chrono::steady_clock::now();
    //             auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
    //             if (elapsed_time >= time_limit) {
    //                 std::cout << "[RAP] 时间限制已到，提前终止迭代。" << std::endl;
    //                 break;
    //             }
    //         }

    //         if (max_eq_viol < tol && max_ineq_viol < tol) {
    //             std::cout << "[RAP] 收敛于 iter=" << t << ", eq_viol=" << max_eq_viol << ", ineq_viol=" << max_ineq_viol << std::endl;
    //             break;
    //         }
    //     }

    //     // 7) 输出结果
    //     write_solution(sol_file, var_list, x);
    // }

    void sls_model::solve_with_rap(const std::string& lp_file_path, const std::string& sol_file, ModelMode mode, const std::chrono::steady_clock::time_point& start_time, int time_limit)
    {
        const double alpha0 = 0.00001;
        const double tol = 1e-6;
        const double inf_bound = 1e8;

        // -----------------------
        // 解析 LP 文件（与原实现相同）
        std::ifstream ifs(lp_file_path);
        if (!ifs.is_open()) {
            std::cerr << "[RAP] 无法打开LP文件: " << lp_file_path << std::endl;
            return;
        }
        std::ostringstream oss;
        oss << ifs.rdbuf();
        std::string text = oss.str();
        ifs.close();

        std::string ltext = to_lower_str(text);
        auto pos_of = [&](const std::string &kw)->int {
            auto p = ltext.find(to_lower_str(kw));
            return p == std::string::npos ? -1 : (int)p;
        };
        int pMin = pos_of("minimize");
        int pSub = pos_of("subject to");
        int pBounds = pos_of("bounds");
        int pGen = pos_of("generals");
        int pEnd = pos_of("end");

        auto section = [&](int a, int b)->std::string {
            if (a < 0) return "";
            if (b < 0) b = (int)text.size();
            if (b <= a) return "";
            return text.substr(a, b-a);
        };

        std::string minimize_sec = section(pMin, (pSub>0? pSub : (pBounds>0? pBounds : (pGen>0? pGen : pEnd))));
        std::string subject_sec  = section(pSub, (pBounds>0? pBounds : (pGen>0? pGen : pEnd)));
        std::string bounds_sec   = section(pBounds, (pGen>0? pGen : pEnd));
        std::string generals_sec = section(pGen, pEnd);

        // 2) 读取 LP 文件并获取变量、目标系数、二次系数、上下界等（按项注册变量）
        std::unordered_map<std::string,int> var_index;
        std::vector<std::string> var_list;
        std::vector<double> c;
        std::vector<double> w;
        std::vector<double> lb;
        std::vector<double> ub;

        auto is_keyword = [&](const std::string &s)->bool {
            std::string sl = to_lower_str(s);
            if (sl=="minimize" || sl=="subject" || sl=="to" || sl=="bounds" || sl=="generals" || sl=="end") return true;
            if (sl=="obj" || sl=="obj:") return true;
            return false;
        };

        auto ensure_var = [&](const std::string &name)->int {
            if (name.empty()) return -1;
            std::string nl = name;
            if (is_keyword(nl)) return -1;
            std::string nl_low = to_lower_str(nl);
            if (nl_low == "inf") return -1;
            auto it = var_index.find(nl);
            if (it != var_index.end()) return it->second;
            int id = (int)var_list.size();
            var_index[nl] = id;
            var_list.push_back(nl);
            c.push_back(0.0);
            w.push_back(0.0);
            lb.push_back(0.0);
            ub.push_back(inf_bound);
            return id;
        };

        if (!minimize_sec.empty()) {
            size_t lbpos = minimize_sec.find('[');
            size_t rbpos = std::string::npos;
            if (lbpos != std::string::npos) rbpos = minimize_sec.find(']', lbpos+1);
            if (lbpos != std::string::npos && rbpos != std::string::npos) {
                std::string quad_body = minimize_sec.substr(lbpos+1, rbpos - lbpos - 1);
                std::regex quad_term_re(R"(([+-]?\s*(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)?\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:\^?2)?)");
                auto it = std::sregex_iterator(quad_body.begin(), quad_body.end(), quad_term_re);
                auto end = std::sregex_iterator();
                for (; it != end; ++it) {
                    std::smatch m = *it;
                    std::string coef_s = m.size()>1 ? m[1].str() : "";
                    std::string varname = m.size()>2 ? m[2].str() : "";
                    if (varname.empty()) continue;
                    int id = ensure_var(varname);
                    if (id < 0) continue;
                    double coef = 1.0;
                    if (!coef_s.empty()) {
                        coef_s.erase(remove_if(coef_s.begin(), coef_s.end(), ::isspace), coef_s.end());
                        try { coef = std::stod(coef_s); } catch(...) { continue; }
                    }
                    w[id] += coef;
                }
            }
            std::string linear_part = minimize_sec;
            if (lbpos != std::string::npos && rbpos != std::string::npos) {
                linear_part.erase(lbpos, rbpos - lbpos + 1);
            }
            std::regex lin_term_re(R"(([+-]?\s*(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)?\s*([A-Za-z_][A-Za-z0-9_]*))");
            {
                auto it = std::sregex_iterator(linear_part.begin(), linear_part.end(), lin_term_re);
                auto end = std::sregex_iterator();
                for (; it != end; ++it) {
                    std::smatch m = *it;
                    std::string coef_s = m.size()>1 ? m[1].str() : "";
                    std::string varname = m.size()>2 ? m[2].str() : "";
                    if (varname.empty()) continue;
                    int id = ensure_var(varname);
                    if (id < 0) continue;
                    double coef = 1.0;
                    if (!coef_s.empty()) {
                        coef_s.erase(remove_if(coef_s.begin(), coef_s.end(), ::isspace), coef_s.end());
                        try { coef = std::stod(coef_s); } catch(...) { continue; }
                    }
                    c[id] += coef;
                }
            }
        }

        if (!bounds_sec.empty()) {
            std::stringstream ss(bounds_sec);
            std::string line;
            std::regex bound_re(R"(([-+eE0-9\.infINF]+)\s*<=\s*([A-Za-z_][A-Za-z0-9_]*)\s*<=\s*([-\+eE0-9\.infINF]+))", std::regex::icase);
            while (std::getline(ss, line)) {
                line = trim_str(line);
                if (line.empty()) continue;
                std::smatch m;
                if (std::regex_search(line, m, bound_re)) {
                    std::string left = m[1].str();
                    std::string varname = m[2].str();
                    std::string right = m[3].str();
                    int id = ensure_var(varname);
                    if (id < 0) continue;
                    auto parseb = [&](const std::string &s)->double {
                        std::string t = s;
                        t.erase(remove_if(t.begin(), t.end(), ::isspace), t.end());
                        std::string tl = to_lower_str(t);
                        if (tl.find("inf") != std::string::npos) return inf_bound;
                        try { return std::stod(t); } catch(...) { return 0.0; }
                    };
                    lb[id] = parseb(left);
                    ub[id] = parseb(right);
                }
            }
        }

        std::vector<std::vector<double>> A_eq;
        std::vector<double> b_eq;
        std::vector<std::vector<double>> A_ineq;
        std::vector<double> b_ineq;

        if (!subject_sec.empty()) {
            std::stringstream ss(subject_sec);
            std::string raw;
            std::regex rel_re(R"(<=|>=|=)");
            std::regex term_re(R"(([+-]?\s*(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)?\s*([A-Za-z_][A-Za-z0-9_]*))");
            while (std::getline(ss, raw)) {
                std::string line = trim_str(raw);
                if (line.empty()) continue;
                size_t colpos = line.find(':');
                if (colpos != std::string::npos) line = trim_str(line.substr(colpos+1));
                std::smatch rm;
                if (!std::regex_search(line, rm, rel_re)) continue;
                std::string rel = rm.str(0);
                size_t rpos = line.find(rel);
                std::string lhs = trim_str(line.substr(0, rpos));
                std::string rhs = trim_str(line.substr(rpos + rel.size()));
                double rhs_val = 0.0;
                try { rhs_val = std::stod(rhs); } catch(...) { continue; }

                std::vector<double> coeffs(var_list.size(), 0.0);
                auto it = std::sregex_iterator(lhs.begin(), lhs.end(), term_re);
                auto end = std::sregex_iterator();
                for (; it != end; ++it) {
                    std::smatch m = *it;
                    std::string coef_s = m.size()>1 ? m[1].str() : "";
                    std::string varname = m.size()>2 ? m[2].str() : "";
                    if (varname.empty()) continue;
                    double coef = 1.0;
                    if (!coef_s.empty()) {
                        coef_s.erase(remove_if(coef_s.begin(), coef_s.end(), ::isspace), coef_s.end());
                        try { coef = std::stod(coef_s); } catch(...) { coef = 1.0; }
                    }
                    int id = ensure_var(varname);
                    if (id < 0) continue;
                    if ((int)coeffs.size() < (int)var_list.size()) coeffs.resize(var_list.size(), 0.0);
                    coeffs[id] += coef;
                }

                if (rel == "=") {
                    A_eq.push_back(coeffs);
                    b_eq.push_back(rhs_val);
                } else if (rel == ">=") {
                    A_ineq.push_back(coeffs);
                    b_ineq.push_back(rhs_val);
                } else {
                    for (int j=0;j<(int)coeffs.size();++j) coeffs[j] = -coeffs[j];
                    A_ineq.push_back(coeffs);
                    b_ineq.push_back(-rhs_val);
                }
            }
        }

        int n = (int)var_list.size();
        expand_rows_to_n(A_eq, n);
        expand_rows_to_n(A_ineq, n);

        int m_eq = (int)A_eq.size();
        int m_ineq = (int)A_ineq.size();

        // -----------------------
        // 转换为稀疏行表示
        // -----------------------
        auto dense_to_sparse_rows = [&](const std::vector<std::vector<double>>& A_dense)
            -> std::vector<std::vector<std::pair<int,double>>> {
            std::vector<std::vector<std::pair<int,double>>> rows;
            rows.reserve(A_dense.size());
            for (const auto &row : A_dense) {
                std::vector<std::pair<int,double>> srow;
                for (int j = 0; j < (int)row.size(); ++j) {
                    double v = row[j];
                    if (v != 0.0) srow.emplace_back(j, v);
                }
                rows.emplace_back(std::move(srow));
            }
            return rows;
        };

        std::vector<std::vector<std::pair<int,double>>> Aeq_rows = dense_to_sparse_rows(A_eq);
        std::vector<std::vector<std::pair<int,double>>> Aineq_rows = dense_to_sparse_rows(A_ineq);

        // 3) 初始化对偶与原始变量
        std::vector<double> x(n, 0.0);
        for (int j=0;j<n;++j) {
            if (std::isfinite(lb[j]) && lb[j] > -inf_bound/2) x[j] = lb[j];
            else x[j] = 0.0;
        }
        std::vector<double> mu(m_ineq, 0.0);
        std::vector<double> lam(m_eq, 0.0);

        // r = c + A_eq^T * lam + A_ineq^T * mu
        // lam/mu 初始为 0，因此 r 初始化为 c
        std::vector<double> r = c;

        std::cout << "[RAP] 开始对偶子梯度迭代（改进版：稀疏+单遍历更新 r）（时间限制：" << time_limit << "秒）" << std::endl;

        for (int t = 0; ; ++t) {
            double alpha = alpha0 / std::sqrt((double)(t + 1));

            // 用当前 r 求 x（逐分量解析）
            std::vector<double> x_prev = x;
            for (int j = 0; j < n; ++j) {
                if (w[j] > 0.0) {
                    double zj = - r[j] / w[j];
                    if (zj < lb[j]) zj = lb[j];
                    if (zj > ub[j]) zj = ub[j];
                    x[j] = zj;
                } else {
                    if (r[j] > 0.0) x[j] = lb[j];
                    else if (r[j] < 0.0) x[j] = ub[j];
                    else x[j] = x_prev[j];
                }
            }

            // 单次稀疏行遍历：计算 s = A * x - b，并直接把对偶增量的 A^T * delta 加到 r
            std::vector<double> s_eq(m_eq, 0.0);
            std::vector<double> s_ineq(m_ineq, 0.0);

            // 等式行
            for (int i = 0; i < m_eq; ++i) {
                double Ax = 0.0;
                const auto &row = Aeq_rows[i];
                for (const auto &p : row) Ax += p.second * x[p.first];
                double si = Ax - b_eq[i];
                s_eq[i] = si;
                double delta_lam = alpha * si;
                lam[i] += delta_lam;
                if (delta_lam != 0.0) {
                    for (const auto &p : row) r[p.first] += p.second * delta_lam;
                }
            }

            // 不等式行（mu 做非负投影）
            for (int i = 0; i < m_ineq; ++i) {
                double Ax = 0.0;
                const auto &row = Aineq_rows[i];
                for (const auto &p : row) Ax += p.second * x[p.first];
                double si = Ax - b_ineq[i];
                s_ineq[i] = si;

                double mu_old = mu[i];
                double mu_candidate = mu_old + alpha * si;
                if (mu_candidate < 0.0) mu_candidate = 0.0;
                double delta_mu = mu_candidate - mu_old;
                if (delta_mu != 0.0) {
                    for (const auto &p : row) r[p.first] += p.second * delta_mu;
                    mu[i] = mu_candidate;
                }
            }

            // 收敛判定（基于残差）
            double max_eq_viol = 0.0;
            for (int i = 0; i < m_eq; ++i) max_eq_viol = std::max(max_eq_viol, std::fabs(s_eq[i]));
            double max_ineq_viol = 0.0;
            for (int i = 0; i < m_ineq; ++i) {
                double viol = std::max(0.0, -s_ineq[i]); // s_ineq = Ax - b, 需 b - Ax 的正部分
                if (viol > max_ineq_viol) max_ineq_viol = viol;
            }

            if (t % 1000 == 0) {
                double quad = 0.0;
                for (int j = 0; j < n; ++j) quad += w[j] * x[j] * x[j];
                double obj = 0.5 * quad;
                for (int j = 0; j < n; ++j) obj += c[j] * x[j];
                std::cout << "[RAP] iter=" << t << " obj=" << obj
                        << " max_eq_viol=" << max_eq_viol
                        << " max_ineq_viol=" << max_ineq_viol << std::endl;

                auto current_time = std::chrono::steady_clock::now();
                auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                if (elapsed_time >= time_limit) {
                    std::cout << "[RAP] 时间限制已到，提前终止迭代。" << std::endl;
                    break;
                }
            }

            if (max_eq_viol < tol && max_ineq_viol < tol) {
                std::cout << "[RAP] 收敛于 iter=" << t << ", eq_viol=" << max_eq_viol << ", ineq_viol=" << max_ineq_viol << std::endl;
                break;
            }
        }

        write_solution(sol_file, var_list, x);
    }


    void sls_model::solve_with_gurobi(const std::string& lp_file_path, ModelMode mode, int time_limit, int method, int threads)
    {
        std::cout << "开始使用 Gurobi 求解: " << lp_file_path << std::endl;
        
        // 根据 ModelMode 生成模式后缀
        std::string mode_suffix;
        switch (mode) {
            case ZeroObj:
                mode_suffix = "zeroobj";
                break;
            case HeatObj:
                mode_suffix = "heatobj";
                break;
            case GivenQueryObj:
                mode_suffix = "givenqueryobj";
                break;
            case RandQueryObj:
                mode_suffix = "randqueryobj";
                break;
            case MaxQueryObj:
                mode_suffix = "maxqueryobj";
                break;
            default:
                mode_suffix = "unknown";
                break;
        }
        
        // 路径映射：从 lp_data 路径映射到 lp_solve/Gurobi 路径
        std::string result_path = lp_file_path;
        std::string log_path = lp_file_path;
        
        // 查找并替换路径中的 "lp_data" 为 "lp_solve/Gurobi"
        size_t lp_data_pos = result_path.find("lp_data");
        if (lp_data_pos != std::string::npos) {
            result_path.replace(lp_data_pos, strlen("lp_data"), "lp_solve/Gurobi");
            log_path = result_path;
        }
        
        // 生成参数后缀
        std::string param_suffix = "_time" + std::to_string(time_limit) + 
                                  "_method" + std::to_string(method) + 
                                  "_thread" + std::to_string(threads) + 
                                  "_" + mode_suffix;
        
        // 生成结果文件路径（在 .lp 前插入参数后缀，然后替换为 .sol）
        size_t lp_ext_pos = result_path.find(".lp");
        if (lp_ext_pos != std::string::npos) {
            result_path.insert(lp_ext_pos, param_suffix);
            result_path.replace(result_path.find(".lp"), 3, ".sol");
        }
        
        // 生成日志文件路径（在 .lp 前插入参数后缀，然后替换为 .log）
        lp_ext_pos = log_path.find(".lp");
        if (lp_ext_pos != std::string::npos) {
            log_path.insert(lp_ext_pos, param_suffix);
            log_path.replace(log_path.find(".lp"), 3, ".log");
        }
        
        // 创建输出目录
        std::string result_dir = result_path.substr(0, result_path.find_last_of('/'));
        std::string mkdir_cmd = "mkdir -p \"" + result_dir + "\"";
        int mkdir_ret = system(mkdir_cmd.c_str());
        if (mkdir_ret != 0) {
            std::cerr << "警告: 创建目录失败: " << result_dir << std::endl;
        }
        
        // 构建 Gurobi 命令
        std::string gurobi_cmd = "gurobi_cl TimeLimit=" + std::to_string(time_limit) + 
                                " ResultFile=\"" + result_path + 
                                "\" Method=" + std::to_string(method) + 
                                " Threads=" + std::to_string(threads) + 
                                " LogFile=\"" + log_path + "\" \"" + lp_file_path + "\"";
        
        std::cout << "执行 Gurobi 命令: " << gurobi_cmd << std::endl;
        std::cout << "模式: " << mode_suffix << std::endl;
        std::cout << "参数: TimeLimit=" << time_limit << ", Method=" << method << ", Threads=" << threads << std::endl;
        
        // 执行 Gurobi 求解
        int ret = system(gurobi_cmd.c_str());
        
        if (ret == 0) {
            std::cout << "Gurobi 求解完成" << std::endl;
            std::cout << "结果文件: " << result_path << std::endl;
            std::cout << "日志文件: " << log_path << std::endl;
        } else {
            std::cerr << "Gurobi 求解失败，返回码: " << ret << std::endl;
        }
    }
};