// knapsack_generator.h — генератор входных данных для задачи о рюкзаке 0-1
#pragma once
#include <cstdint>
#include <random>
#include <vector>

struct KnapsackInstance {
    int n = 0;              // число предметов
    int W = 0;              // вместимость рюкзака
    std::vector<int> w;     // веса:      1 <= w[i] <= W
    std::vector<int> c;     // стоимости: 1 <= c[i] <= C_MAX
};

class KnapsackGenerator {
public:
    static constexpr int C_MAX = 1000;

    explicit KnapsackGenerator(std::uint32_t seed) : rng_(seed) {}

    // Случайный вход: веса и стоимости независимы
    // и равномерно распределены (некоррелированный класс)
    KnapsackInstance random(int n, int W) {
        std::uniform_int_distribution<int> weight(1, W);
        std::uniform_int_distribution<int> cost(1, C_MAX);
        KnapsackInstance d{n, W, std::vector<int>(n), std::vector<int>(n)};
        for (int i = 0; i < n; ++i) {
            d.w[i] = weight(rng_);
            d.c[i] = cost(rng_);
        }
        return d;
    }

    // Наихудший случай (п. 2.4): все w[i] = 1
    KnapsackInstance worst(int n, int W) { return fixedWeights(n, W, 1); }

    // Наилучший случай при 1 <= w[i] <= W: все w[i] = W
    KnapsackInstance best(int n, int W) { return fixedWeights(n, W, W); }

private:
    KnapsackInstance fixedWeights(int n, int W, int weight) {
        std::uniform_int_distribution<int> cost(1, C_MAX);
        KnapsackInstance d{n, W, std::vector<int>(n, weight),
                           std::vector<int>(n)};
        for (int& ci : d.c) ci = cost(rng_);
        return d;
    }

    std::mt19937 rng_;      // вихрь Мерсенна MT19937
};
