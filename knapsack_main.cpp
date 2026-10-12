// knapsack_main.cpp — решение одной индивидуальной задачи о рюкзаке.
//
// Формат входа (файл или стандартный ввод):
//   n W
//   w_1 c_1
//   ...
//   w_n c_n
//
// Запуск:  ./knapsack [файл]
// Вывод: оптимальная стоимость F(n, W), номера взятых предметов,
// их суммарный вес, число базовых операций (с проверкой по формуле (4))
// и время работы (медиана 5 запусков версии без счётчика).
#include "knapsack.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <istream>
#include <stdexcept>

namespace {

knapsack::Instance read_instance(std::istream& is) {
    knapsack::Instance in;
    if (!(is >> in.n >> in.W) || in.n < 0 || in.W < 0) {
        throw std::runtime_error("ошибка чтения n и W");
    }
    in.w.assign(static_cast<std::size_t>(in.n) + 1, 0);
    in.c.assign(static_cast<std::size_t>(in.n) + 1, 0);
    for (int i = 1; i <= in.n; ++i) {
        if (!(is >> in.w[i] >> in.c[i])) {
            throw std::runtime_error("ошибка чтения предмета " +
                                     std::to_string(i));
        }
    }
    return in;
}

// Время одного запуска Knapsack_DP без счётчика, мкс.
double run_time_us(const knapsack::Instance& in, std::int32_t& value) {
    const auto start = std::chrono::steady_clock::now();
    const knapsack::DpTable F = knapsack::knapsack_dp(in);
    const auto finish = std::chrono::steady_clock::now();
    // Результат используется, поэтому компилятор не удалит вызов.
    value = knapsack::optimal_value(F);
    return std::chrono::duration<double, std::micro>(finish - start).count();
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        knapsack::Instance in;
        if (argc > 1) {
            std::ifstream file(argv[1]);
            if (!file) {
                throw std::runtime_error(
                    std::string("не удалось открыть файл ") + argv[1]);
            }
            in = read_instance(file);
        } else {
            in = read_instance(std::cin);
        }

        // 1. Решение со счётчиком операций.
        knapsack::OpCounter ops;
        const knapsack::DpTable F = knapsack::knapsack_dp(in, ops);
        const std::int32_t best = knapsack::optimal_value(F);
        const std::vector<int> items = knapsack::restore_items(in, F);

        long long total_weight = 0;
        for (int i : items) total_weight += in.w[i];

        // 2. Время: 5 запусков без счётчика, медиана.
        constexpr int kRuns = 5;
        std::array<double, kRuns> times{};
        for (double& t : times) {
            std::int32_t value = 0;
            t = run_time_us(in, value);
            if (value != best) {
                throw std::logic_error("результаты запусков не совпали");
            }
        }
        std::nth_element(times.begin(), times.begin() + kRuns / 2, times.end());
        const double median_us = times[kRuns / 2];

        const std::uint64_t predicted = knapsack::predicted_ops(in);

        std::cout << "n = " << in.n << ", W = " << in.W << '\n'
                  << "Оптимальная стоимость F(n, W) = " << best << '\n'
                  << "Взятые предметы:";
        for (int i : items) std::cout << ' ' << i;
        std::cout << "\nСуммарный вес = " << total_weight
                  << " (W = " << in.W << ")\n"
                  << "M(D) = " << knapsack::count_misfits(in) << '\n'
                  << "Базовые операции: измерено " << ops.value
                  << ", по формуле (4) " << predicted
                  << (ops.value == predicted ? " -- совпадает"
                                             : " -- НЕ СОВПАДАЕТ")
                  << '\n'
                  << "Время (медиана " << kRuns << " запусков) = "
                  << median_us << " мкс\n"
                  << "Память под таблицу F = " << F.memory_bytes() << " байт\n";
        return ops.value == predicted ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << '\n';
        return 1;
    }
}
