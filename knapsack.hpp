// knapsack.hpp — алгоритм Knapsack_DP для задачи о рюкзаке 0-1
// (метод динамического программирования) со встроенным счётчиком
// базовых операций.
//
// Реализация следует рекуррентному соотношению из [Левитин, разд. 8.4]
// и [Кормен и др., упр. 16.2-2]; правила подсчёта операций — п. 2.2 отчёта.
// Стандарт языка: C++17.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace knapsack {

// Индивидуальная задача D = (n, W, w_1..w_n, c_1..c_n).
// Массивы индексируются с 1, как в псевдокоде; элемент [0] не используется.
struct Instance {
    int n = 0;
    int W = 0;
    std::vector<int> w;  // веса,      w[1..n]
    std::vector<int> c;  // стоимости, c[1..n]
};

// Проверка корректности входа (в трудоёмкость не входит).
inline void validate(const Instance& in) {
    if (in.n < 0 || in.W < 0) {
        throw std::invalid_argument("n и W должны быть неотрицательными");
    }
    const auto expected = static_cast<std::size_t>(in.n) + 1;
    if (in.w.size() != expected || in.c.size() != expected) {
        throw std::invalid_argument(
            "размеры массивов w и c должны быть равны n + 1");
    }
    for (int i = 1; i <= in.n; ++i) {
        if (in.w[i] < 1 || in.c[i] < 1) {
            throw std::invalid_argument(
                "веса и стоимости должны быть положительными, i = " +
                std::to_string(i));
        }
    }
}

// Таблица F размером (n + 1) x (W + 1), хранимая построчно в одном
// непрерывном массиве: строка i занимает элементы [i(W+1), (i+1)(W+1)).
class DpTable {
public:
    DpTable(int n, int W)
        : n_(n), W_(W),
          cells_((static_cast<std::size_t>(n) + 1) *
                 (static_cast<std::size_t>(W) + 1)) {}

    int n() const { return n_; }
    int W() const { return W_; }

    std::int32_t* row(int i) {
        return cells_.data() + offset(i);
    }
    const std::int32_t* row(int i) const {
        return cells_.data() + offset(i);
    }

    std::int32_t at(int i, int j) const { return row(i)[j]; }

    std::size_t memory_bytes() const {
        return cells_.size() * sizeof(std::int32_t);
    }

private:
    std::size_t offset(int i) const {
        return static_cast<std::size_t>(i) * (static_cast<std::size_t>(W_) + 1);
    }

    int n_;
    int W_;
    std::vector<std::int32_t> cells_;
};

// Счётчик базовых операций.
struct OpCounter {
    std::uint64_t value = 0;
    void add(std::uint64_t k) { value += k; }
};

// Пустой счётчик: вызовы add() удаляются компилятором при оптимизации,
// поэтому версия для замера времени не содержит операций подсчёта.
struct NoCounter {
    void add(std::uint64_t) {}
};

// Алгоритм Knapsack_DP. Заполняет таблицу F по рекуррентному соотношению
//   F(0, j) = 0,
//   F(i, j) = F(i-1, j),                              если w_i > j,
//   F(i, j) = max{F(i-1, j), F(i-1, j-w_i) + c_i},    если w_i <= j.
// Комментарии справа — стоимость строки псевдокода в базовых
// операциях (п. 2.2).
template <class Counter>
DpTable knapsack_dp(const Instance& in, Counter& ops) {
    validate(in);
    const int n = in.n;
    const int W = in.W;
    const std::vector<int>& w = in.w;
    const std::vector<int>& c = in.c;

    DpTable F(n, W);

    ops.add(1);  // For j <- 0
    std::int32_t* first = F.row(0);
    for (int j = 0; j <= W; ++j) {
        ops.add(3);  // проверка, инкремент, присваивание
        first[j] = 0;
        ops.add(3);  // F[0][j] <- 0
    }

    ops.add(1);  // For i <- 1
    for (int i = 1; i <= n; ++i) {
        ops.add(3);
        const std::int32_t* prev = F.row(i - 1);
        std::int32_t* cur = F.row(i);
        const int wi = w[i];
        const std::int32_t ci = c[i];

        ops.add(1);  // For j <- 0
        for (int j = 0; j <= W; ++j) {
            ops.add(3);
            ops.add(2);  // If w[i] > j
            if (wi > j) {
                cur[j] = prev[j];
                ops.add(6);  // ветвь then
            } else {
                cur[j] = std::max(prev[j], prev[j - wi] + ci);
                ops.add(14);  // ветвь else
            }
        }
    }

    ops.add(2);  // Return F[n][W]
    return F;
}

// Перегрузка без подсчёта операций — для измерения времени.
inline DpTable knapsack_dp(const Instance& in) {
    NoCounter none;
    return knapsack_dp(in, none);
}

// Оптимальное значение целевой функции (1): F(n, W).
inline std::int32_t optimal_value(const DpTable& F) {
    return F.at(F.n(), F.W());
}

// Восстановление оптимального набора обратным проходом от ячейки (n, W):
// если F(i, j) != F(i-1, j), предмет i взят и j уменьшается на w_i.
// Возвращает номера взятых предметов по возрастанию. В трудоёмкость T(D)
// не входит: псевдокод п. 2.2 возвращает только значение F[n][W].
inline std::vector<int> restore_items(const Instance& in,
                                      const DpTable& F) {
    std::vector<int> items;
    int j = in.W;
    for (int i = in.n; i >= 1; --i) {
        if (F.at(i, j) != F.at(i - 1, j)) {
            items.push_back(i);
            j -= in.w[i];
        }
    }
    std::reverse(items.begin(), items.end());
    return items;
}

// M(D) — число пар (i, j), для которых w_i > j; формула (3).
inline std::uint64_t count_misfits(const Instance& in) {
    const std::uint64_t cap = static_cast<std::uint64_t>(in.W) + 1;
    std::uint64_t M = 0;
    for (int i = 1; i <= in.n; ++i) {
        const auto wi = static_cast<std::uint64_t>(in.w[i]);
        M += std::min(wi, cap);
    }
    return M;
}

// Теоретическая трудоёмкость по формуле (4):
//   T(D) = 19n(W+1) + 4n + 6(W+1) + 4 - 8M(D).
inline std::uint64_t predicted_ops(const Instance& in) {
    const std::uint64_t n = static_cast<std::uint64_t>(in.n);
    const std::uint64_t W1 = static_cast<std::uint64_t>(in.W) + 1;
    return 19 * n * W1 + 4 * n + 6 * W1 + 4 - 8 * count_misfits(in);
}

}  // namespace knapsack
