#! /usr/bin/env python3

import argparse
import difflib
import fcntl
import os
import pty
import selectors
import shlex
import shutil
import string
import struct
import subprocess
import sys
import termios
from itertools import islice
from multiprocessing import get_context
from pathlib import Path
from typing import Generator
from typing import NamedTuple

from create_test_folders import create_test_folders
from gen_data import ALLOWED_FLAGS, gen_data


class PtyResult(NamedTuple):
    stdout: str
    stderr: str
    returncode: int


class TestBatch(NamedTuple):
    term_size: int
    ft_bin: str
    cases: list[list[str]]


class TestFailure(NamedTuple):
    term_size: int
    cmd: list[str]
    kind: str
    diff: str


DEBUG = True
TERMINAL_MIN = 80  # include
TERMINAL_MAX = 250  # exclude
BATCH_SIZE = 32


def main() -> None:
    args = parse_args()
    test_path = Path.cwd().joinpath("ft_ls_tester")
    if test_path.exists():
        remove_test_root(test_path)

    # subprocess.run("make fclean", shell=True, capture_output=True)
    # compile_ls(term_size=80, debug=args.debug)
    # print("=" * 60)
    # print("Phase 1: Invalid Flags")
    # print("=" * 60)
    # invalid_flags(debug=args.debug)

    try:
        data = create_test_folders(path=test_path)
        cases = list(gen_data(paths=data))
        for term_size in range(TERMINAL_MIN, TERMINAL_MAX, 1):
            # subprocess.run("make fclean", shell=True, capture_output=True)
            print("=" * 60)
            compile_ls(term_size=term_size, debug=args.debug)
            print(
                "Flag Combinations and Feature Tests",
                f"(jobs={args.jobs})",
            )
            print("=" * 60)

            failure = run_cases_parallel(
                cases=cases, term_size=term_size, jobs=args.jobs, debug=args.debug
            )

            if failure is not None:
                print(format_failure(failure), file=sys.stderr)
                sys.exit(1)
            break
    except Exception as e:
        print(e, file=sys.stderr)
        sys.exit(1)

    remove_test_root(test_path)

    print("=" * 60)
    print("All tests passed!")
    print("=" * 60)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-d",
        "--debug",
        action="store_true",
        help="Runs the debug version of ft_ls. (called ft_ls_d)",
    )

    parser.add_argument(
        "-j",
        "--jobs",
        type=positive_int,
        default=default_jobs(),
        help="worker processes to use for test execution",
    )

    return parser.parse_args()


def positive_int(value: str) -> int:
    jobs = int(value)
    if jobs < 1:
        raise argparse.ArgumentTypeError("jobs must be at least 1")

    return jobs


def default_jobs() -> int:
    cpu_count = os.cpu_count() or 1
    return max(1, cpu_count // 2)


def remove_test_root(test_path: Path) -> None:
    if not test_path.exists():
        return

    for root, dirs, files in os.walk(test_path, topdown=False):
        for name in files:
            try:
                Path(root, name).chmod(0o666)
            except (FileNotFoundError, PermissionError):
                pass
        for name in dirs:
            try:
                Path(root, name).chmod(0o777)
            except (FileNotFoundError, PermissionError):
                pass

    try:
        test_path.chmod(0o777)
    except (FileNotFoundError, PermissionError):
        pass

    shutil.rmtree(test_path)


def compile_ls(term_size: int, debug: bool) -> None:
    print("", "-" * 5, "Compile with column width:", term_size, "-" * 5)
    if debug:
        result = subprocess.run(
            f"make TERM_SIZE={term_size} debug",
            shell=True,
            capture_output=True,
            text=True,
        )
    else:
        result = subprocess.run(
            f"make TERM_SIZE={term_size}", shell=True, capture_output=True, text=True
        )

    assert result.returncode == 0, f"Compilation failed: {result.stderr}"


def invalid_flags(debug: bool) -> None:
    own_bin = "./ft_ls"
    if debug:
        own_bin = f"{own_bin}_d"

    counter = 0
    for lttr in string.ascii_letters:
        if lttr in ALLOWED_FLAGS:
            continue

        result = subprocess.run(
            f"{own_bin} -{lttr}", shell=True, capture_output=True, text=True
        )

        try:
            assert result.returncode == 1
        except AssertionError:
            msg = f"{own_bin} -{lttr} should return exit code 1, goth: {result.returncode}"
            raise AssertionError(msg)

        try:
            assert result.stdout == ""
        except AssertionError:
            msg = f"{own_bin} -{lttr} should have empty stdout, goth: {result.stdout}"
            raise AssertionError(msg)

        try:
            assert result.stderr != ""
        except AssertionError:
            msg = f"{own_bin} -{lttr} should have stderr message, goth: nothing..."
            raise AssertionError(msg)

        counter += 1

    print(f"  Tested {counter} invalid flags: all passed")


def run_cases_parallel(
    cases: list[list[str]], term_size: int, jobs: int, debug: bool
) -> TestFailure | None:
    own_bin = "./ft_ls"
    if debug:
        own_bin = f"{own_bin}_d"

    if jobs == 1:
        return run_case_batch(
            TestBatch(term_size=term_size, ft_bin=own_bin, cases=cases)
        )

    ctx = get_context("fork")
    with ctx.Pool(processes=jobs) as pool:
        batches = iter_case_batches(
            cases=cases, term_size=term_size, ft_bin=own_bin, batch_size=BATCH_SIZE
        )

        for failure in pool.imap_unordered(run_case_batch, batches, chunksize=1):
            if failure is not None:
                pool.terminate()
                return failure

    return None


def iter_case_batches(
    cases: list[list[str]], term_size: int, ft_bin: str, batch_size: int
) -> Generator[TestBatch, None, None]:
    iterator = iter(cases)
    while True:
        batch_cases = list(islice(iterator, batch_size))
        if not batch_cases:
            return None

        yield TestBatch(term_size=term_size, ft_bin=ft_bin, cases=batch_cases)


def run_case_batch(batch: TestBatch) -> TestFailure | None:
    for args in batch.cases:
        failure = compare_case(
            term_size=batch.term_size, ft_bin=batch.ft_bin, args=args
        )

        if failure is not None:
            return failure

    return None


def compare_case(term_size: int, ft_bin: str, args: list[str]) -> TestFailure | None:
    ft_cmd = [ft_bin, *args]
    ls_cmd = ["ls", *args]

    ft = run_with_pty(cmd=ft_cmd, cols=term_size)
    ls = run_with_pty(cmd=ls_cmd, cols=term_size)

    diff = diff_output(ft.stdout, ls.stdout)
    if diff is not None:
        return TestFailure(term_size=term_size, cmd=ft_cmd, kind="stdout", diff=diff)

    diff = diff_output(ft.stderr, ls.stderr, normalize_program=True)
    if diff is not None:
        return TestFailure(term_size=term_size, cmd=ft_cmd, kind="stderr", diff=diff)

    if ft.returncode != ls.returncode:
        return TestFailure(
            term_size=term_size,
            cmd=ft_cmd,
            kind="returncode",
            diff=(f"ft_ls returncode: {ft.returncode}\nls returncode: {ls.returncode}"),
        )

    return None


def normalize_program_name(output: str) -> str:
    lines: list[str] = []

    for line in output.splitlines(keepends=True):
        if line.startswith("ft_ls_d:"):
            lines.append(line.replace("ft_ls_d", "ls", 1))
        elif line.startswith("ft_ls:"):
            lines.append(line.replace("ft_ls", "ls", 1))
        elif line.startswith("usage: ft_ls "):
            lines.append(line.replace("usage: ft_ls ", "usage: ls ", 1))
        else:
            lines.append(line)

    return "".join(lines)


def diff_output(
    ft_out: str, ls_out: str, normalize_program: bool = False
) -> str | None:
    ft_out_cpy = normalize_program_name(ft_out) if normalize_program else ft_out
    if ft_out_cpy == ls_out:
        return None

    diff = "".join(
        difflib.unified_diff(
            ft_out_cpy.splitlines(keepends=True),
            ls_out.splitlines(keepends=True),
            fromfile="ft_ls",
            tofile="ls",
        )
    )

    if diff:
        return diff

    return f"ft_ls: {ft_out_cpy!r}\nls: {ls_out!r}"


def format_failure(failure: TestFailure) -> str:
    lines = [
        "=" * 60,
        "Test failed",
        f"TERM_SIZE: {failure.term_size}",
        f"CMD: {shlex.join(failure.cmd)}",
        f"KIND: {failure.kind}",
        "DIFF:",
        failure.diff.rstrip("\n"),
        "=" * 60,
    ]

    return "\n".join(lines)


def run_with_pty(cmd: list[str], cols: int = 80) -> PtyResult:
    """Run command in a PTY with specified terminal width."""
    stdout_master, stdout_slave = pty.openpty()
    stderr_master, stderr_slave = pty.openpty()

    winsize = struct.pack("HHHH", 24, cols, 0, 0)
    fcntl.ioctl(stdout_slave, termios.TIOCSWINSZ, winsize)
    fcntl.ioctl(stderr_slave, termios.TIOCSWINSZ, winsize)
    env = os.environ.copy()
    env["LC_ALL"] = "C"
    env["LANG"] = "C"
    env.pop("COLUMNS", None)
    env["TERM"] = "screen-256color"

    proc = subprocess.Popen(
        cmd,
        stdout=stdout_slave,
        stderr=stderr_slave,
        stdin=stdout_slave,
        close_fds=True,
        env=env,
    )

    os.close(stdout_slave)
    os.close(stderr_slave)

    selector = selectors.DefaultSelector()
    selector.register(stdout_master, selectors.EVENT_READ, "stdout")
    selector.register(stderr_master, selectors.EVENT_READ, "stderr")
    chunks: dict[str, list[str]] = {"stdout": [], "stderr": []}

    while selector.get_map():
        for key, _ in selector.select():
            try:
                data = os.read(key.fd, 1024)
            except OSError:
                data = b""

            if not data:
                selector.unregister(key.fd)
                os.close(key.fd)
                continue

            chunks[key.data].append(data.decode())

    returncode = proc.wait()
    selector.close()

    return PtyResult(
        stdout="".join(chunks["stdout"]).replace("\r", ""),
        stderr="".join(chunks["stderr"]).replace("\r", ""),
        returncode=returncode,
    )


if __name__ == "__main__":
    main()
