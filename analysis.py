"""analysis.py — рисунки главы 7: отношение T_ср / g и удельное время.

Запуск:  python3 analysis.py <каталог с series1..3.csv> <каталог для рисунков>
"""
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import matplotlib.ticker as mticker  # noqa: E402
import numpy as np  # noqa: E402

from figures import ACCENT, FIG_SIZE, INK, MAIN, MUTED, read_series, ru_formatter

MIB = 1024 * 1024


def g(n, W):
    """Функция класса сложности g(n, W) = n(W + 1)."""
    return n * (W + 1)


def t_mean_theory(n, W):
    """Средняя трудоёмкость по формуле (5)."""
    return 15 * n * W + 19 * n + 6 * (W + 1) + 4


def plot_ratio(s1, s2, path):
    fig, ax = plt.subplots(figsize=FIG_SIZE)
    for rows, color, marker, label in ((s1, MAIN, "o", "серия 1 ($W$ = 1000)"),
                                       (s2, ACCENT, "s", "серия 2 ($W = n$)")):
        n = np.array([r["n"] for r in rows], dtype=float)
        W = np.array([r["W"] for r in rows], dtype=float)
        T = np.array([r["T_mean"] for r in rows])
        ax.plot(n, T / g(n, W), linestyle="none", marker=marker, markersize=3.5,
                color=color, label=label + ", эксперимент")
        ax.plot(n, t_mean_theory(n, W) / g(n, W), color=color, linewidth=1.0,
                alpha=0.8, label=label + ", формула (5)")
    ax.axhline(15, color=MUTED, linewidth=0.8, linestyle="--")
    ax.set_xlabel("Число предметов $n$")
    ax.set_ylabel("$T_{\\mathrm{ср}}\\,/\\,(n(W+1))$")
    ax.set_xlim(0, 6600)
    ax.set_ylim(14.95, 15.12)
    ax.xaxis.set_major_locator(mticker.MultipleLocator(800))
    ax.xaxis.set_major_formatter(ru_formatter())
    ax.yaxis.set_major_formatter(ru_formatter(2))
    ax.legend(loc="upper right", fontsize=9)
    fig.savefig(path)
    plt.close(fig)


def plot_time_per_cell(series, path):
    fig, ax = plt.subplots(figsize=FIG_SIZE)
    styles = ((MAIN, "o", "серия 1"), (ACCENT, "s", "серия 2"), (INK, "^", "серия 3"))
    for rows, (color, marker, label) in zip(series, styles):
        n = np.array([r["n"] for r in rows], dtype=float)
        W = np.array([r["W"] for r in rows], dtype=float)
        t_ns = np.array([r["time_mean_us"] for r in rows]) * 1000
        size_mib = 4 * (n + 1) * (W + 1) / MIB
        ax.plot(size_mib, t_ns / g(n, W), linestyle="none", marker=marker,
                markersize=4, color=color, label=label)
    ax.axvline(32, color=MUTED, linewidth=0.8, linestyle="--")
    ax.text(34, 0.25, "32 МиБ", color=MUTED, fontsize=9)
    ax.set_xscale("log", base=2)
    ax.set_xlabel("Объём таблицы $F$, МиБ (лог. шкала)")
    ax.set_ylabel("Время на одну ячейку, нс")
    ax.set_ylim(0, 3.6)
    ax.xaxis.set_major_formatter(ru_formatter(2, strip_zeros=True))
    ax.yaxis.set_major_formatter(ru_formatter(1))
    ax.legend(loc="upper left")
    fig.savefig(path)
    plt.close(fig)


def main():
    src, out_dir = Path(sys.argv[1]), Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)
    s1, s2, s3 = (read_series(src / f"series{k}.csv") for k in (1, 2, 3))
    plot_ratio(s1, s2, out_dir / "fig7_1.png")
    plot_time_per_cell((s1, s2, s3), out_dir / "fig7_2.png")


if __name__ == "__main__":
    main()
