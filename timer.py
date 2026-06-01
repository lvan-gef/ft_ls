#! /usr/bin/env python3

import subprocess
import time
from itertools import combinations


def bench(cmd: list[str], runs: int):
    durations = []

    for _ in range(runs):
        start = time.perf_counter()
        subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        durations.append(time.perf_counter() - start)

    return durations


def print_stats(name: str, durations: list[float]):
    average = sum(durations) / len(durations)
    fastest = min(durations)
    slowest = max(durations)
    print(f'{name}: avg={average:.6f}s min={fastest:.6f}s max={slowest:.6f}s')
    return average


def run():
    p = '/media/luuk/essd'

    flags = ['R', 'a', 'l', 'r', 't']
    cmd_own = ['./ft_ls', p]
    cmd_ls = ['ls', p]
    runs = 20

    for size in range(1, len(flags) + 1):
        for combo in combinations(flags, size):
            flag = f'-{''.join(combo)}'
            print('cache warmup for ls')
            subprocess.run(cmd_ls, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
            print(f'benchmarking {runs} runs for ls')
            ls_results = bench(cmd=['ls', flag, p], runs=runs)

            print('cache warmup for ft_ls')
            subprocess.run(cmd_own, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
            print(f'benchmarking {runs} runs for ft_ls')
            own_results = bench(['./ft_ls', flag, p], runs=runs)

            # - above 1.000x means ft_ls is slower
            # - below 1.000x means ft_ls is faster
            ls_avg = print_stats('ls   ', ls_results)
            own_avg = print_stats('ft_ls', own_results)
            print(f'ratio: ft_ls / ls {flag} = {own_avg / ls_avg:.3f}x\n')


run()
