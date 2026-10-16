// gen.cpp — запись R случайных входов в текстовый файл
// Использование: gen n W seed R > input.txt
#include <cstdlib>
#include <iostream>
#include "knapsack_generator.h"

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "usage: gen n W seed R\n";
        return 1;
    }
    const int n = std::atoi(argv[1]);
    const int W = std::atoi(argv[2]);
    const auto seed =
        static_cast<std::uint32_t>(std::strtoul(argv[3], nullptr, 10));
    const int R = std::atoi(argv[4]);

    KnapsackGenerator gen(seed);
    // std::cout << R << '\n';
    for (int r = 0; r < R; ++r) {
        const KnapsackInstance d = gen.random(n, W);
        std::cout << d.n << ' ' << d.W << '\n';
        for (int i = 0; i < d.n; ++i)
            std::cout << d.w[i] << ' ' << d.c[i] << '\n';
    }
    return 0;
}
