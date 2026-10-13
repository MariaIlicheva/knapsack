// experiment.cpp — вычислительный эксперимент для алгоритма Knapsack_DP
// (глава 6 отчёта). Серии и их параметры — таблица 3.2 отчёта.
//
//   Серия 1: W = 1000,        n = 100k,    k = 1, ..., 64;
//   Серия 2: n = W = 100k,                 k = 1, ..., 64;
//   Серия 3: n = 1000,        W = 2^m - 1, m = 7, ..., 16 (k = m);
//   Серия 4: (n, W) = (100, 100) при k = 1 и (1000, 1000) при k = 2.
//
// В каждой точке серий 1-3 решается R = 100 случайных входов и два
// контрольных входа (w_i = 1 и w_i = W); в серии 4 — N = 10 000 случайных
// входов. Начальное значение генератора точки s = 1000 S + k (п. 4.1.3).
// Для каждого входа показание счётчика сверяется с формулой (4), для
// контрольных входов — с формулами T^ и Tv из п. 2.4.
//
// Запуск:  ./experiment S файл.csv [--no-time]
//   S = 1, 2, 3 — в файл пишется по строке на точку серии;
//   S = 4       — в файл пишется трудоёмкость каждого входа.
#include "knapsack.hpp"
#include "knapsack_generator.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int kRandomInputs = 100;      // R, серии 1-3
constexpr int kHistogramInputs = 10000; // N, серия 4
constexpr int kTimeRuns = 5;            // запусков для медианы времени

struct Point {
    int k;  // номер точки в серии
    int n;
    int W;
};

std::vector<Point> series_points(int S) {
    std::vector<Point> points;
    switch (S) {
        case 1:
            for (int k = 1; k <= 64; ++k) points.push_back({k, 100 * k, 1000});
            break;
        case 2:
            for (int k = 1; k <= 64; ++k) points.push_back({k, 100 * k, 100 * k});
            break;
        case 3:
            for (int m = 7; m <= 16; ++m) points.push_back({m, 1000, (1 << m) - 1});
            break;
        case 4:
            points.push_back({1, 100, 100});
            points.push_back({2, 1000, 1000});
            break;
        default:
            throw std::invalid_argument("номер серии должен быть от 1 до 4");
    }
    return points;
}

// Перевод входа генератора (индексы с 0) во вход алгоритма (индексы с 1).
knapsack::Instance to_instance(const KnapsackInstance& d) {
    knapsack::Instance in;
    in.n = d.n;
    in.W = d.W;
    in.w.assign(static_cast<std::size_t>(d.n) + 1, 0);
    in.c.assign(static_cast<std::size_t>(d.n) + 1, 0);
    std::copy(d.w.begin(), d.w.end(), in.w.begin() + 1);
    std::copy(d.c.begin(), d.c.end(), in.c.begin() + 1);
    return in;
}

// Число базовых операций с проверкой по формуле (4).
std::uint64_t count_ops(const knapsack::Instance& in) {
    knapsack::OpCounter ops;
    const knapsack::DpTable F = knapsack::knapsack_dp(in, ops);
    (void)F;
    if (ops.value != knapsack::predicted_ops(in)) {
        throw std::logic_error("счётчик не совпал с формулой (4)");
    }
    return ops.value;
}

// Сюда записывается ответ, чтобы компилятор не удалил замеряемый вызов.
volatile std::int32_t g_sink = 0;

// Медиана времени kTimeRuns запусков без счётчика, мкс (п. 3.5).
double median_time_us(const knapsack::Instance& in) {
    std::array<double, kTimeRuns> times{};
    for (double& t : times) {
        const auto start = std::chrono::steady_clock::now();
        const knapsack::DpTable F = knapsack::knapsack_dp(in);
        const auto finish = std::chrono::steady_clock::now();
        g_sink = knapsack::optimal_value(F);
        t = std::chrono::duration<double, std::micro>(finish - start).count();
    }
    std::nth_element(times.begin(), times.begin() + kTimeRuns / 2, times.end());
    return times[kTimeRuns / 2];
}

// Выборочное среднее и стандартное отклонение (алгоритм Уэлфорда).
class RunningStats {
public:
    void add(double x) {
        ++count_;
        const double delta = x - mean_;
        mean_ += delta / count_;
        m2_ += delta * (x - mean_);
    }
    double mean() const { return mean_; }
    double stddev() const {
        return count_ > 1 ? std::sqrt(m2_ / (count_ - 1)) : 0.0;
    }

private:
    long long count_ = 0;
    double mean_ = 0.0;
    double m2_ = 0.0;
};

// Формулы п. 2.4: T^ (все w_i = 1) и Tv (все w_i = W).
std::uint64_t worst_formula(std::uint64_t n, std::uint64_t W) {
    return 19 * n * (W + 1) - 4 * n + 6 * (W + 1) + 4;
}
std::uint64_t best_formula(std::uint64_t n, std::uint64_t W) {
    return 11 * n * W + 23 * n + 6 * (W + 1) + 4;
}

void run_series(int S, std::ostream& out, bool with_time) {
    out << "series,k,seed,n,W,R,T_mean,T_std,T_min,T_max,"
           "T_worst,T_best,time_mean_us,time_std_us\n";
    for (const Point& p : series_points(S)) {
        const auto seed = static_cast<std::uint32_t>(1000 * S + p.k);
        KnapsackGenerator gen(seed);

        RunningStats ops_stats;
        RunningStats time_stats;
        std::uint64_t t_min = UINT64_MAX;
        std::uint64_t t_max = 0;
        for (int r = 0; r < kRandomInputs; ++r) {
            const knapsack::Instance in = to_instance(gen.random(p.n, p.W));
            const std::uint64_t T = count_ops(in);
            ops_stats.add(static_cast<double>(T));
            t_min = std::min(t_min, T);
            t_max = std::max(t_max, T);
            if (with_time) time_stats.add(median_time_us(in));
        }

        // Контрольные входы.
        const std::uint64_t T_worst = count_ops(to_instance(gen.worst(p.n, p.W)));
        const std::uint64_t T_best = count_ops(to_instance(gen.best(p.n, p.W)));
        if (T_worst != worst_formula(p.n, p.W) || T_best != best_formula(p.n, p.W)) {
            throw std::logic_error("контрольный вход не совпал с формулами п. 2.4");
        }

        out.precision(12);
        out << S << ',' << p.k << ',' << seed << ',' << p.n << ',' << p.W << ','
            << kRandomInputs << ',' << ops_stats.mean() << ','
            << ops_stats.stddev() << ',' << t_min << ',' << t_max << ','
            << T_worst << ',' << T_best << ',' << time_stats.mean() << ','
            << time_stats.stddev() << '\n';
        std::cerr << "серия " << S << ": n = " << p.n << ", W = " << p.W
                  << " — готово\n";
    }
}

void run_histograms(std::ostream& out) {
    out << "k,seed,n,W,r,M,T\n";
    for (const Point& p : series_points(4)) {
        const auto seed = static_cast<std::uint32_t>(4000 + p.k);
        KnapsackGenerator gen(seed);
        for (int r = 1; r <= kHistogramInputs; ++r) {
            const knapsack::Instance in = to_instance(gen.random(p.n, p.W));
            const std::uint64_t T = count_ops(in);
            out << p.k << ',' << seed << ',' << p.n << ',' << p.W << ',' << r
                << ',' << knapsack::count_misfits(in) << ',' << T << '\n';
        }
        std::cerr << "серия 4: n = " << p.n << ", W = " << p.W << " — готово\n";
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc < 3) {
            std::cerr << "usage: experiment S file.csv [--no-time]\n";
            return 1;
        }
        const int S = std::atoi(argv[1]);
        const bool with_time = !(argc > 3 && std::string(argv[3]) == "--no-time");
        std::ofstream out(argv[2]);
        if (!out) throw std::runtime_error(std::string("не удалось открыть ") + argv[2]);
        if (S == 4) {
            run_histograms(out);
        } else {
            run_series(S, out, with_time);
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << '\n';
        return 1;
    }
}
