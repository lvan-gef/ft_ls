import subprocess
import time


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
    cmd_own = ['./ft_ls', '-R', '/home/luuk']
    cmd_ls = ['ls', '-R', '/home/luuk']
    runs = 20


    print('cache warmup for ls')
    subprocess.run(cmd_ls, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    print(f'benchmarking {runs} runs for ls')
    ls_results = bench(cmd_ls, runs=runs)

    print('cache warmup for ft_ls')
    subprocess.run(cmd_own, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    print(f'benchmarking {runs} runs for ft_ls')
    own_results = bench(cmd_own, runs=runs)

    # - above 1.000x means ft_ls is slower
    # - below 1.000x means ft_ls is faster
    ls_avg = print_stats('ls   ', ls_results)
    own_avg = print_stats('ft_ls', own_results)
    print(f'ratio: ft_ls / ls  -R = {own_avg / ls_avg:.3f}x')

    cmd_own = ['./ft_ls', '-Rl', '/home/luuk']
    cmd_ls = ['ls', '-Rl', '/home/luuk']
    runs = 20


    print('cache warmup for ls')
    subprocess.run(cmd_ls, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    print(f'benchmarking {runs} runs for ls')
    ls_results = bench(cmd_ls, runs=runs)

    print('cache warmup for ft_ls')
    subprocess.run(cmd_own, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    print(f'benchmarking {runs} runs for ft_ls')
    own_results = bench(cmd_own, runs=runs)

    # - above 1.000x means ft_ls is slower
    # - below 1.000x means ft_ls is faster
    ls_avg = print_stats('ls   ', ls_results)
    own_avg = print_stats('ft_ls', own_results)
    print(f'ratio: ft_ls / ls Rl= {own_avg / ls_avg:.3f}x')

run()
