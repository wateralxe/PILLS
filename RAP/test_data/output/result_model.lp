Minimize
 obj: 3.1708e+07 p_6 + 3.1708e+07 p_4 + 3.1708e+07 p_3 + 3.1708e+07 p_5 + 3.1708e+07 p_2 + 3.1708e+07 p_1 + 3.1708e+07 p_0 + [ x_0_0^2 + 0.4x_1_1^2 + 0.2x_2_2^2 + 0.8x_3_3^2 + 0.6x_4_4^2 + 0.4x_5_5^2 ] / 2

Subject To
user_supply_sum_user5_supply5_equal:  1 x_0_0_0 -1 x_0_0 = 0
user_supply_user5_supply5_upper:  1 x_0_0_0 <= 40
user_supply_sum_user6_supply6_equal:  1 x_1_1_1 -1 x_1_1 = 0
user_supply_user6_supply6_upper:  1 x_1_1_1 <= 60
user_supply_sum_user3_supply3_equal:  1 x_2_2_2 -1 x_2_2 = 0
user_supply_user3_supply3_upper:  1 x_2_2_2 <= 30
user_supply_sum_user4_supply4_equal:  1 x_3_3_3 + 1 x_3_3_1 -1 x_3_3 = 0
user_supply_user4_supply4_upper:  1 x_3_3_3 + 1 x_3_3_1 <= 80
user_supply_sum_user2_supply2_equal:  1 x_4_4_4 + 1 x_4_4_5 -1 x_4_4 = 0
user_supply_user2_supply2_upper:  1 x_4_4_4 + 1 x_4_4_5 <= 50
user_supply_sum_user1_supply1_equal:  1 x_5_5_6 + 1 x_5_5_2 -1 x_5_5 = 0
user_supply_user1_supply1_upper:  1 x_5_5_6 + 1 x_5_5_2 <= 100
demand_2_lower:  1 x_5_5_6 + 1 p_6 >= 3
demand_4_lower:  1 x_4_4_4 + 1 p_4 >= 1
demand_6_lower:  1 x_3_3_3 + 1 p_3 >= 2
demand_3_lower:  1 x_4_4_5 + 1 p_5 >= 2
demand_1_lower:  1 x_2_2_2 + 1 x_5_5_2 + 1 p_2 >= 4
demand_5_lower:  1 x_1_1_1 + 1 x_3_3_1 + 1 p_1 >= 5
demand_7_lower:  1 x_0_0_0 + 1 p_0 >= 3

Bounds
 0 <= x_0_0_0 <= 40
 0 <= x_0_0 <= 40
 0 <= x_1_1_1 <= 60
 0 <= x_1_1 <= 60
 0 <= x_2_2_2 <= 30
 0 <= x_2_2 <= 30
 0 <= x_3_3_3 <= 80
 0 <= x_3_3_1 <= 80
 0 <= x_3_3 <= 80
 0 <= x_4_4_4 <= 50
 0 <= x_4_4_5 <= 50
 0 <= x_4_4 <= 50
 0 <= x_5_5_6 <= 100
 0 <= x_5_5_2 <= 100
 0 <= x_5_5 <= 100
 0 <= p_6 <= inf
 0 <= p_4 <= inf
 0 <= p_3 <= inf
 0 <= p_5 <= inf
 0 <= p_2 <= inf
 0 <= p_1 <= inf
 0 <= p_0 <= inf

Generals
 x_0_0_0 x_0_0 x_1_1_1 x_1_1 x_2_2_2 x_2_2 x_3_3_3 x_3_3_1 x_3_3 x_4_4_4 x_4_4_5 x_4_4 x_5_5_6 x_5_5_2 x_5_5 p_6 p_4 p_3 p_5 p_2 p_1 p_0
End
