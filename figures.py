"""figures.py — графики и гистограммы главы 6 по результатам experiment.

Запуск:  python3 figures.py <каталог с series1..4.csv> <каталог для рисунков>
Создаёт рисунки PNG (300 dpi) и файл summary.json с таблицами главы 6.
"""
import csv
import json
import math
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import matplotlib.ticker as mticker  # noqa: E402
import numpy as np  # noqa: E402

INK = "#1f1f1f"
MUTED = "#6b6b6b"
GRID = "#d9d9d9"
MAIN = "#2a5c8a"     # основная серия
ACCENT = "#c0504d"   # выделенные точки (удвоение n)

plt.rcParams.update({
    "font.family": "Liberation Serif",  # метрически совместим с Times New Roman
    "font.size": 11,
    "axes.edgecolor": MUTED,
    "axes.labelcolor": INK,
    "axes.linewidth": 0.8,
    "xtick.color": INK,
    "ytick.color": INK,
    "xtick.labelcolor": INK,
    "ytick.labelcolor": INK,
    "axes.grid": True,
    "grid.color": GRID,
    "grid.linewidth": 0.6,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "legend.frameon": False,
    "savefig.dpi": 300,
    "savefig.bbox": "tight",
    "axes.formatter.use_locale": False,
    "mathtext.fontset": "custom",
    "mathtext.rm": "Liberation Serif",
    "mathtext.it": "Liberation Serif:italic",
    "mathtext.bf": "Liberation Serif:bold",
})

FIG_SIZE = (6.3, 3.3)  # дюймы: ширина текста страницы A4 ≈ 16 см


def ru_number(x, decimals=0):
    """Число в русской записи: пробел между разрядами, запятая в дробной части."""
    s = f"{x:,.{decimals}f}".replace(",", " ").replace(".", ",")
    return s


def ru_formatter(decimals=0, strip_zeros=False):
    def fmt(x, _pos):
        text = ru_number(x, decimals)
        if strip_zeros and "," in text:
            text = text.rstrip("0").rstrip(",")
        return text
    return mticker.FuncFormatter(fmt)


def read_series(path):
    with open(path, encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        for key in ("k", "n", "W", "R", "T_min", "T_max", "T_worst", "T_best", "seed"):
            r[key] = int(r[key])
        for key in ("T_mean", "T_std", "time_mean_us", "time_std_us"):
            r[key] = float(r[key])
    return rows


def plot_vs_n(rows, value_key, scale, ylabel, path, doubling_ns):
    n = np.array([r["n"] for r in rows])
    y = np.array([r[value_key] for r in rows]) / scale
    fig, ax = plt.subplots(figsize=FIG_SIZE)
    ax.plot(n, y, color=MAIN, linewidth=1.6, marker="o", markersize=3,
            label="все точки серии ($n$ = 100, 200, …, 6400)")
    mask = np.isin(n, doubling_ns)
    ax.plot(n[mask], y[mask], linestyle="none", marker="o", markersize=7,
            markerfacecolor=ACCENT, markeredgecolor="white", markeredgewidth=1.2,
            label="точки удвоения $n = 100 \\cdot 2^k$")
    ax.set_xlabel("Число предметов $n$")
    ax.set_ylabel(ylabel)
    ax.set_xlim(0, 6600)
    ax.set_ylim(bottom=0)
    ax.xaxis.set_major_locator(mticker.MultipleLocator(800))
    ax.xaxis.set_major_formatter(ru_formatter())
    ax.yaxis.set_major_formatter(ru_formatter(0 if y.max() >= 20 else 1))
    ax.legend(loc="upper left")
    fig.savefig(path)
    plt.close(fig)


def plot_vs_m(rows, value_key, scale, ylabel, path):
    m = np.array([r["k"] for r in rows])
    y = np.array([r[value_key] for r in rows]) / scale
    fig, ax = plt.subplots(figsize=FIG_SIZE)
    ax.plot(m, y, color=MAIN, linewidth=1.6, marker="o", markersize=6,
            markerfacecolor=MAIN, markeredgecolor="white", markeredgewidth=1.0)
    ax.set_yscale("log", base=2)
    ax.set_xlabel("Число битов $m$ в записи вместимости $W = 2^m - 1$")
    ax.set_ylabel(ylabel)
    ax.set_xticks(m)
    ax.yaxis.set_major_formatter(ru_formatter(2, strip_zeros=True))
    ax.yaxis.set_minor_formatter(mticker.NullFormatter())
    fig.savefig(path)
    plt.close(fig)


def histogram(T, n, W, path):
    """Гистограмма относительных частот: k интервалов по правилу Стёрджеса,
    ширина интервала кратна 8 (шаг значений T, формула (4))."""
    N = len(T)
    k = int(1 + math.log2(N))  # 1 + log2(10 000) = 14,3 -> 14
    t_min, t_max = int(T.min()), int(T.max())
    h = math.ceil((t_max - t_min + 1) / k / 8) * 8
    edges = t_min + h * np.arange(k + 1)
    counts = np.array([np.sum((T >= edges[j]) & (T < edges[j + 1])) for j in range(k)])
    assert counts.sum() == N
    freq = counts / N

    fig, ax = plt.subplots(figsize=FIG_SIZE)
    centers = (edges[:-1] + edges[1:]) / 2
    scale = 1e3 if t_max < 1e6 else 1e6
    unit = "тыс." if scale == 1e3 else "млн"
    ax.bar(centers / scale, freq, width=h / scale * 0.94, color=MAIN,
           edgecolor="none", zorder=2)
    mean = T.mean()
    ax.vlines(mean / scale, 0, freq.max() * 1.06, color=ACCENT, linewidth=1.6,
              linestyle="--", zorder=3,
               label=f"выборочное среднее $T_{{\\mathrm{{ср}}}}$ = {ru_number(mean, 0)}")
    ax.set_xlabel(f"Трудоемкость $T$, {unit} базовых операций")
    ax.set_ylabel("Относительная частота")
    ax.xaxis.set_major_locator(mticker.MaxNLocator(steps=[1, 2, 5, 10]))
    ax.xaxis.set_major_formatter(ru_formatter(0 if scale == 1e3 else 2))
    ax.yaxis.set_major_formatter(ru_formatter(2))
    ax.grid(axis="x", visible=False)
    ax.set_ylim(0, freq.max() * 1.32)
    ax.legend(loc="upper left")
    fig.savefig(path)
    plt.close(fig)

    # Выборочные коэффициенты асимметрии и эксцесса через центральные
    # моменты m3, m4 и смещённое отклонение sigma_в (Гмурман, гл. 17).
    dev = T - mean
    sigma_b = math.sqrt(np.mean(dev ** 2))
    return {
        "n": n, "W": W, "N": N, "k": k, "h": h,
        "edges": edges.tolist(), "counts": counts.tolist(), "freq": freq.tolist(),
        "mean": float(mean), "std": float(T.std(ddof=1)),
        "min": t_min, "max": t_max, "distinct": int(len(np.unique(T))),
        "median": float(np.median(T)),
        "skew": float(np.mean(dev ** 3) / sigma_b ** 3),
        "kurt": float(np.mean(dev ** 4) / sigma_b ** 4 - 3),
    }


def main():
    data_dir, out_dir = Path(sys.argv[1]), Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)
    summary = {}
    doubling = [100 * 2 ** k for k in range(7)]

    if (data_dir / "series1.csv").exists():
        s1 = read_series(data_dir / "series1.csv")
        plot_vs_n(s1, "T_mean", 1e6, "$T_{\\mathrm{ср}}$, млн операций", out_dir / "fig6_1.png", doubling)
        plot_vs_n(s1, "time_mean_us", 1e3, "Время, мс", out_dir / "fig6_2.png", doubling)
        summary["series1"] = s1
    if (data_dir / "series2.csv").exists():
        s2 = read_series(data_dir / "series2.csv")
        plot_vs_n(s2, "T_mean", 1e6, "$T_{\\mathrm{ср}}$, млн операций", out_dir / "fig6_3.png", doubling)
        plot_vs_n(s2, "time_mean_us", 1e3, "Время, мс", out_dir / "fig6_4.png", doubling)
        summary["series2"] = s2
    if (data_dir / "series3.csv").exists():
        s3 = read_series(data_dir / "series3.csv")
        plot_vs_m(s3, "T_mean", 1e6, "$T_{\\mathrm{ср}}$, млн операций (лог. шкала)", out_dir / "fig6_5.png")
        plot_vs_m(s3, "time_mean_us", 1e3, "Время, мс (лог. шкала)", out_dir / "fig6_6.png")
        summary["series3"] = s3
    if (data_dir / "series4.csv").exists():
        with open(data_dir / "series4.csv", encoding="utf-8") as f:
            rows = list(csv.DictReader(f))
        hist = []
        for k, name in (("1", "fig6_7.png"), ("2", "fig6_8.png")):
            sel = [r for r in rows if r["k"] == k]
            T = np.array([int(r["T"]) for r in sel], dtype=np.int64)
            hist.append(histogram(T, int(sel[0]["n"]), int(sel[0]["W"]), out_dir / name))
        summary["series4"] = hist

    with open(out_dir / "summary.json", "w", encoding="utf-8") as f:
        json.dump(summary, f, ensure_ascii=False, indent=1)


if __name__ == "__main__":
    main()
