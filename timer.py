#! /usr/bin/env python3

import statistics
import subprocess
import time
from itertools import combinations
from pathlib import Path
from shutil import which


def time_batch(cmd: list[str], repeat: int) -> float:
    start = time.perf_counter()

    for _ in range(repeat):
        subprocess.run(
            cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False
        )

    return (time.perf_counter() - start) / repeat


def bench(
    ls_cmd: list[str], ft_cmd: list[str], runs: int, batch_size: int
) -> tuple[list[float], list[float]]:
    ls_durations = []
    ft_durations = []

    for index in range(runs):
        if index % 2 == 0:
            ls_durations.append(time_batch(cmd=ls_cmd, repeat=batch_size))
            ft_durations.append(time_batch(cmd=ft_cmd, repeat=batch_size))
        else:
            ft_durations.append(time_batch(cmd=ft_cmd, repeat=batch_size))
            ls_durations.append(time_batch(cmd=ls_cmd, repeat=batch_size))

    return ls_durations, ft_durations


def print_stats(name: str, durations: list[float]) -> float:
    average = sum(durations) / len(durations)
    median = statistics.median(durations)
    fastest = min(durations)
    slowest = max(durations)
    print(
        f"{name}: median={median:.6f}s avg={average:.6f}s min={fastest:.6f}s max={slowest:.6f}s"
    )
    return median


def run():
    p = "/mnt/bulk2"

    flags = ["R", "l"]
    # flags = ["R", "a", "l", "r", "t"]
    runs = 21
    batch_size = 5
    ls_bin = which("ls")
    ft_bin = str(Path("./ft_ls_bonus").resolve())

    if ls_bin is None:
        raise RuntimeError("could not find ls in PATH")

    for size in range(1, len(flags) + 1):
        for combo in combinations(flags, size):
            flag = f"-{''.join(combo)}"
            print("cache warmup")
            bench(
                ls_cmd=[ls_bin, flag, p], ft_cmd=[ft_bin, flag, p], runs=1, batch_size=1
            )

            ls_results, ft_results = bench(
                ls_cmd=[ls_bin, flag, p],
                ft_cmd=[ft_bin, flag, p],
                runs=runs,
                batch_size=batch_size,
            )

            # - above 1.000x means ft_ls is slower
            # - below 1.000x means ft_ls is faster
            ls_median = print_stats("ls   ", ls_results)
            own_median = print_stats("ft_ls", ft_results)
            print(
                f"ratio: ft_ls / ls {flag} = {own_median / ls_median:.3f}x (median)\n"
            )


run()
