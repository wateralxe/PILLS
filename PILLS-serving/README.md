./main <demand_file> <offline_supply_file> <serve_supply_file> <offline_res_csv> <serving_result_csv>

示例：
./main ../test_data/test_demand.txt ../test_data/test_offline_supply.txt ../test_data/test_serving_supply.txt ../test_data/test_offline_res.csv ../test_data/test_serving_res.csv

/pub/data/lijy/alimama/WWW-all/final/competitors/PILLS/PILLS-serving/src/main  /pub/data/lijy/alimama/WWW-all/final/exp_data/demand/offline/0416_01.txt  /pub/data/lijy/alimama/WWW-all/final/exp_data/supply/offline/0416_01.txt  /pub/data/lijy/alimama/WWW-all/final/exp_data/supply/serving/0416_01.txt  /pub/data/lijy/alimama/WWW-all/final/exp_res/offline/HWM/0416_01/offline.csv /pub/data/lijy/alimama/WWW-all/final/exp_res/serving/HWM-PILLS/0416_01/serving.csv

流程：
- 读入 offline_supply：用于校验 offline_res_csv 合法性，并积累历史分配权重。
- 读入 serve_supply：这是实际待分配的供给集合，可能与 offline_supply 不同。
- 读入 demand：只记录在供给阶段出现过的 demand 的需求量。
- 读入 offline_res_csv：校验 sid/did 是否在 offline_supply 中、pv 是否为整数、是否超额，并累计历史分配。
- 按概率给 serve_supply 的 sid 分配 demand：若该 sid 有历史权重则按历史归一化；否则在 serve_supply 允许的 demand 集合上均匀分布。
- 输出分配方案到 serving_result_csv。

# 概率分布策略（offline vs serve 拆分）
假设：

offline 分配历史：
offline_sid_did_allocatepv[SID_A] = { DID_X: 30, DID_Y: 10 }
offline_sid_did_allocatepv[SID_B] = { DID_X: 5, DID_Z: 5 }

serve 允许集合：
serve_sid_did[SID_A] = { DID_X, DID_Y, DID_Z }
serve_sid_did[SID_C] = { DID_Y, DID_Z }

计算对每个 serve sid 的概率：
- 取该 sid 的 serve 允许集合作为可分配域。
- 取对应的 offline 历史分配（如果有），并与可分配域求交集得到权重；若无历史或交集为空，则均匀分布在可分配域。

示例：
- SID_A：历史 {X:30, Y:10}，允许 {X,Y,Z} ⇒ 归一化得 P(X)=0.75, P(Y)=0.25。
- SID_C：无历史，允许 {Y,Z} ⇒ 均匀分布 P(Y)=0.5, P(Z)=0.5。
