#include <iostream>
#include "../include/basic_types.hpp"

int main() {
    // Test position_info::get_real() method
    lt::position_info pos;
    pos.total_long.postion = 100;
    pos.total_short.postion = 50;
    int32_t real = pos.get_real();
    std::cout << "position_info::get_real() = " << real << std::endl;
    if (real == 50) {
        std::cout << "✓ position_info::get_real() fix successful!" << std::endl;
    } else {
        std::cout << "✗ position_info::get_real() fix failed!" << std::endl;
    }

    // Test bar_info::get_delta() method
    lt::bar_info bar;
    bar.buy_details[100.0] = 10;
    bar.buy_details[101.0] = 20;
    bar.sell_details[99.0] = 5;
    bar.sell_details[98.0] = 8;
    int32_t delta = bar.get_delta();
    std::cout << "bar_info::get_delta() = " << delta << std::endl;
    if (delta == 17) { // 10+20 - (5+8) = 30-13=17
        std::cout << "✓ bar_info::get_delta() fix successful!" << std::endl;
    } else {
        std::cout << "✗ bar_info::get_delta() fix failed!" << std::endl;
    }

    return 0;
}
