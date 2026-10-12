// tests.cpp — проверка корректности реализации Knapsack_DP.
//
//  1. Пример из учебника Левитина (разд. 8.4): W = 5, ответ 37.
//  2. Граничные случаи: n = 0, W = 0, все предметы тяжелее W.
//  3. Сравнение с полным перебором на 10 000 случайных входах (n <= 15).
//  4. Допустимость и стоимость восстановленного набора предметов.
//  5. Совпадение счётчика операций с формулой (4) на всех входах,
//     в том числе с формулами T^ (w_i = 1) и Tv (w_i = W) из п. 2.4.
//  6. Независимость трудоёмкости от стоимостей и порядка предметов.
#include "knapsack.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

int g_failed = 0;
int g_passed = 0;

void check(bool condition, const std::string& what) {
    if (condition) {
        ++g_passed;
    } else {
        ++g_failed;
        std::cerr << "FAIL: " << what << '\n';
    }
}

knapsack::Instance make(int W, const std::vector<std::pair<int, int>>& items) {
    knapsack::Instance in;
    in.n = static_cast<int>(items.size());
    in.W = W;
    in.w.assign(items.size() + 1, 0);
    in.c.assign(items.size() + 1, 0);
    for (std::size_t k = 0; k < items.size(); ++k) {
        in.w[k + 1] = items[k].first;
        in.c[k + 1] = items[k].second;
    }
    return in;
}

// Полный перебор 2^n подмножеств — эталон для сравнения.
std::int32_t brute_force(const knapsack::Instance& in) {
    std::int32_t best = 0;
    for (std::uint32_t mask = 0; mask < (1u << in.n); ++mask) {
        long long weight = 0;
        std::int32_t value = 0;
        for (int i = 1; i <= in.n; ++i) {
            if (mask & (1u << (i - 1))) {
                weight += in.w[i];
                value += in.c[i];
            }
        }
        if (weight <= in.W) best = std::max(best, value);
    }
    return best;
}

std::uint64_t measured_ops(const knapsack::Instance& in) {
    knapsack::OpCounter ops;
    knapsack::knapsack_dp(in, ops);
    return ops.value;
}

void test_levitin_example() {
    // Предметы (вес, стоимость): (2, 12), (1, 10), (3, 20), (2, 15); W = 5.
    const auto in = make(5, {{2, 12}, {1, 10}, {3, 20}, {2, 15}});
    const auto F = knapsack::knapsack_dp(in);
    check(knapsack::optimal_value(F) == 37, "пример Левитина: F(4, 5) = 37");
    check(knapsack::restore_items(in, F) == std::vector<int>{1, 2, 4},
          "пример Левитина: набор {1, 2, 4}");
}

void test_edge_cases() {
    const auto empty = make(10, {});
    check(knapsack::optimal_value(knapsack::knapsack_dp(empty)) == 0, "n = 0");
    check(measured_ops(empty) == knapsack::predicted_ops(empty),
          "n = 0: формула (4)");

    const auto zero_cap = make(0, {{1, 5}, {2, 7}});
    check(knapsack::optimal_value(knapsack::knapsack_dp(zero_cap)) == 0,
          "W = 0");

    const auto too_heavy = make(4, {{5, 100}, {9, 200}});
    check(knapsack::optimal_value(knapsack::knapsack_dp(too_heavy)) == 0,
          "все предметы тяжелее W");

    // Проверочный пример п. 2.2: n = 1, W = 0, w_1 = 1 => T = 25.
    check(measured_ops(make(0, {{1, 1}})) == 25, "пример п. 2.2: T = 25");
    // Примеры п. 2.3.2 (класс N): T = 50 и T = 42.
    check(measured_ops(make(1, {{1, 1}})) == 50, "п. 2.3.2: T = 50");
    check(measured_ops(make(1, {{2, 1}})) == 42, "п. 2.3.2: T = 42");
}

void test_random_against_brute_force() {
    std::mt19937 rng(20261012);
    for (int t = 0; t < 10000; ++t) {
        const int n = std::uniform_int_distribution<int>(1, 15)(rng);
        const int W = std::uniform_int_distribution<int>(0, 60)(rng);
        std::uniform_int_distribution<int> weight(1, 30), cost(1, 1000);
        knapsack::Instance in;
        in.n = n;
        in.W = W;
        in.w.assign(n + 1, 0);
        in.c.assign(n + 1, 0);
        for (int i = 1; i <= n; ++i) {
            in.w[i] = weight(rng);
            in.c[i] = cost(rng);
        }

        knapsack::OpCounter ops;
        const auto F = knapsack::knapsack_dp(in, ops);
        const std::int32_t best = knapsack::optimal_value(F);
        const std::string tag = "случайный вход #" + std::to_string(t);

        check(best == brute_force(in), tag + ": совпадение с полным перебором");

        long long weight_sum = 0, value_sum = 0;
        for (int i : knapsack::restore_items(in, F)) {
            weight_sum += in.w[i];
            value_sum += in.c[i];
        }
        check(weight_sum <= W && value_sum == best,
              tag + ": восстановленный набор");
        check(ops.value == knapsack::predicted_ops(in), tag + ": формула (4)");
    }
}

void test_best_and_worst_cases() {
    for (int n : {1, 7, 100}) {
        for (int W : {1, 13, 100}) {
            knapsack::Instance in = make(W, {});
            in.n = n;
            in.w.assign(n + 1, 1);
            in.c.assign(n + 1, 1);
            const std::uint64_t nn = n, W1 = W + 1;
            const std::string params =
                "n = " + std::to_string(n) + ", W = " + std::to_string(W);
            // Наихудший случай: w_i = 1.
            check(measured_ops(in) == 19 * nn * W1 - 4 * nn + 6 * W1 + 4,
                  "T^ при " + params);
            // Наилучший случай при 1 <= w_i <= W: w_i = W.
            std::fill(in.w.begin() + 1, in.w.end(), W);
            check(measured_ops(in) == 11 * nn * W + 23 * nn + 6 * W1 + 4,
                  "Tv при " + params);
        }
    }
}

void test_independence_from_costs_and_order() {
    std::mt19937 rng(7);
    knapsack::Instance in = make(200, {});
    in.n = 50;
    in.w.assign(51, 0);
    in.c.assign(51, 0);
    for (int i = 1; i <= 50; ++i) {
        in.w[i] = std::uniform_int_distribution<int>(1, 200)(rng);
        in.c[i] = std::uniform_int_distribution<int>(1, 1000)(rng);
    }
    const std::uint64_t base = measured_ops(in);

    knapsack::Instance other_costs = in;
    for (int i = 1; i <= 50; ++i) other_costs.c[i] = 1;
    check(measured_ops(other_costs) == base, "T не зависит от стоимостей");

    knapsack::Instance shuffled = in;
    std::vector<int> perm(50);
    for (int i = 0; i < 50; ++i) perm[i] = i + 1;
    std::shuffle(perm.begin(), perm.end(), rng);
    for (int i = 1; i <= 50; ++i) {
        shuffled.w[i] = in.w[perm[i - 1]];
        shuffled.c[i] = in.c[perm[i - 1]];
    }
    check(measured_ops(shuffled) == base, "T не зависит от порядка предметов");
    check(knapsack::optimal_value(knapsack::knapsack_dp(shuffled)) ==
              knapsack::optimal_value(knapsack::knapsack_dp(in)),
          "ответ не зависит от порядка предметов");
}

}  // namespace

int main() {
    test_levitin_example();
    test_edge_cases();
    test_random_against_brute_force();
    test_best_and_worst_cases();
    test_independence_from_costs_and_order();

    std::cout << "Пройдено проверок: " << g_passed
              << ", ошибок: " << g_failed << '\n';
    return g_failed == 0 ? 0 : 1;
}
