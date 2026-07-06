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
import tempfile
import termios
from itertools import islice
from multiprocessing import get_context
from pathlib import Path
from typing import Generator
from typing import NamedTuple

from create_test_folders import create_test_folders
from gen_data import (
    BASE_FLAGS,
    KNOWN_BONUS_FLAGS,
    NON_COMPARABLE_BONUS_FLAGS,
    gen_data,
    gen_non_tty_data,
)


class PtyResult(NamedTuple):
    stdout: str
    stderr: str
    returncode: int


class TestBatch(NamedTuple):
    term_size: int
    ft_bin: str
    cases: list[list[str]]


class TestFailure(NamedTuple):
    term_size: int | None
    cmd: list[str]
    kind: str
    diff: str
    mode: str = "pty"


DEBUG = True
TERMINAL_MIN = 1  # include
TERMINAL_MAX = 250  # exclude
BATCH_SIZE = 32


def main() -> None:
    args = parse_args()
    bonus_mode = bool(args.bonus_flags)
    allowed_flags = BASE_FLAGS + args.bonus_flags
    invalid_allowed_flags = allowed_flags
    if bonus_mode:
        invalid_allowed_flags += NON_COMPARABLE_BONUS_FLAGS
    test_path = Path.cwd().joinpath("ft_ls_tester")
    if test_path.exists():
        remove_test_root(test_path)

    try:
        data = create_test_folders(
            path=test_path,
            include_acl_xattr=bonus_mode,
        )
        cases = list(
            gen_data(
                paths=data,
                allowed_flags=allowed_flags,
                bonus_flags=args.bonus_flags,
            )
        )
        non_tty_cases = (
            list(gen_non_tty_data(paths=data, bonus_flags=args.bonus_flags))
            if bonus_mode
            else []
        )
        checked_invalid_flags = False
        checked_non_tty = False
        for term_size in range(args.cols_start, args.cols_end, args.cols_step):
            print("=" * 60)
            if bonus_mode:
                print("Bonus mode: using runtime PTY width", term_size)
            else:
                compile_ls(term_size=term_size, ft_bin=args.bin)

            if not checked_invalid_flags:
                print("=" * 60)
                print("Phase 1: Invalid Flags")
                print("=" * 60)
                invalid_flags(ft_bin=args.bin, allowed_flags=invalid_allowed_flags)
                checked_invalid_flags = True

            print("=" * 60)
            print(
                "Flag Combinations and Feature Tests",
                f"(bin={args.bin})",
                f"(jobs={args.jobs})",
            )
            print("=" * 60)

            failure = run_cases_parallel(
                cases=cases,
                term_size=term_size,
                jobs=args.jobs,
                ft_bin=args.bin,
            )

            if failure is not None:
                print(format_failure(failure), file=sys.stderr)
                sys.exit(1)

            if bonus_mode and not checked_non_tty:
                print("=" * 60)
                print(
                    "Non-TTY Redirect/Pipe Tests",
                    f"(bin={args.bin})",
                    f"(cases={len(non_tty_cases)})",
                )
                print("=" * 60)
                failure = run_non_tty_cases(cases=non_tty_cases, ft_bin=args.bin)

                if failure is not None:
                    print(format_failure(failure), file=sys.stderr)
                    sys.exit(1)

                checked_non_tty = True
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
        "-b",
        "--bin",
        default=None,
        help="binary to test (default: ./ft_ls, or ./ft_ls_d with --debug)",
    )

    parser.add_argument(
        "-d",
        "--debug",
        action="store_true",
        help="shortcut for --bin ./ft_ls_d",
    )

    parser.add_argument(
        "--cols-start",
        type=positive_int,
        default=TERMINAL_MIN,
        help="inclusive start of terminal column widths to test",
    )

    parser.add_argument(
        "--cols-end",
        type=positive_int,
        default=TERMINAL_MAX,
        help="exclusive end of terminal column widths to test",
    )

    parser.add_argument(
        "--cols-step",
        type=positive_int,
        default=1,
        help="step between terminal column widths",
    )

    parser.add_argument(
        "--bonus-flags",
        type=parse_bonus_flags,
        default=(),
        metavar="FLAGS",
        help="comma-separated or compact GNU-comparable bonus flags, e.g. g,u or gu",
    )

    parser.add_argument(
        "-j",
        "--jobs",
        type=positive_int,
        default=default_jobs(),
        help="worker processes to use for test execution",
    )

    args = parser.parse_args()
    if args.bin is not None and args.debug:
        parser.error("--debug cannot be combined with --bin")

    if args.bin is None:
        args.bin = "./ft_ls_d" if args.debug else "./ft_ls"
    elif args.bin in ("ft_ls", "ft_ls_d"):
        args.bin = f"./{args.bin}"

    if args.cols_end <= args.cols_start:
        parser.error("--cols-end must be greater than --cols-start")

    if not args.bonus_flags:
        assert_base_compile_binary(parser, args.bin)
    else:
        assert_binary_exists(parser, args.bin)

    return args


def parse_bonus_flags(value: str) -> tuple[str, ...]:
    if not value:
        return ()

    raw_flags = value.split(",") if "," in value else list(value)
    flags: list[str] = []
    for raw_flag in raw_flags:
        flag = raw_flag.strip()
        if not flag:
            continue
        if len(flag) != 1:
            raise argparse.ArgumentTypeError(
                f"bonus flag must be one character, got {flag!r}"
            )
        if flag not in KNOWN_BONUS_FLAGS:
            known = ",".join(KNOWN_BONUS_FLAGS)
            raise argparse.ArgumentTypeError(
                f"unknown bonus flag {flag!r}; known flags: {known}"
            )
        if flag in flags:
            raise argparse.ArgumentTypeError(f"duplicate bonus flag {flag!r}")

        flags.append(flag)

    return tuple(flags)


def assert_base_compile_binary(parser: argparse.ArgumentParser, ft_bin: str) -> None:
    if Path(ft_bin).name not in ("ft_ls", "ft_ls_d"):
        parser.error("base mode auto-compiles only ./ft_ls or ./ft_ls_d")


def assert_binary_exists(parser: argparse.ArgumentParser, ft_bin: str) -> None:
    if "/" not in ft_bin and not ft_bin.startswith("."):
        if shutil.which(ft_bin) is None:
            parser.error(f"binary not found in PATH: {ft_bin}")
        return

    path = Path(ft_bin)
    if not path.exists():
        parser.error(f"binary does not exist: {ft_bin}")
    if not os.access(path, os.X_OK):
        parser.error(f"binary is not executable: {ft_bin}")


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


def compile_ls(term_size: int, ft_bin: str) -> None:
    print("", "-" * 5, "Compile with column width:", term_size, "-" * 5)
    subprocess.run(["make", "fclean"], capture_output=True, text=True)
    cmd = ["make", f"TERM_SIZE={term_size}"]
    if Path(ft_bin).name == "ft_ls_d":
        cmd.append("debug")

    result = subprocess.run(cmd, capture_output=True, text=True)

    assert result.returncode == 0, f"Compilation failed: {result.stderr}"


def invalid_flags(ft_bin: str, allowed_flags: tuple[str, ...]) -> None:
    counter = 0
    for lttr in string.ascii_letters:
        if lttr in allowed_flags:
            continue

        result = subprocess.run(
            [ft_bin, f"-{lttr}"], capture_output=True, text=True
        )

        try:
            assert result.returncode == 1
        except AssertionError:
            msg = f"{ft_bin} -{lttr} should return exit code 1, goth: {result.returncode}"
            raise AssertionError(msg)

        try:
            assert result.stdout == ""
        except AssertionError:
            msg = f"{ft_bin} -{lttr} should have empty stdout, goth: {result.stdout}"
            raise AssertionError(msg)

        try:
            assert result.stderr != ""
        except AssertionError:
            msg = f"{ft_bin} -{lttr} should have stderr message, goth: nothing..."
            raise AssertionError(msg)

        counter += 1

    print(f"  Tested {counter} invalid flags: all passed")


def run_cases_parallel(
    cases: list[list[str]], term_size: int, jobs: int, ft_bin: str
) -> TestFailure | None:
    if jobs == 1:
        return run_case_batch(
            TestBatch(term_size=term_size, ft_bin=ft_bin, cases=cases)
        )

    ctx = get_context("fork")
    with ctx.Pool(processes=jobs) as pool:
        batches = iter_case_batches(
            cases=cases, term_size=term_size, ft_bin=ft_bin, batch_size=BATCH_SIZE
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


def run_non_tty_cases(cases: list[list[str]], ft_bin: str) -> TestFailure | None:
    for mode in ("redirect", "pipe"):
        print(f"  {mode}: {len(cases)} cases")
        for args in cases:
            failure = compare_non_tty_case(ft_bin=ft_bin, args=args, mode=mode)
            if failure is not None:
                return failure

    return None


def compare_non_tty_case(
    ft_bin: str, args: list[str], mode: str
) -> TestFailure | None:
    ft_cmd = [ft_bin, *args]
    ls_cmd = ["ls", *args]

    if mode == "redirect":
        ft = run_with_redirect(ft_cmd)
        ls = run_with_redirect(ls_cmd)
    elif mode == "pipe":
        ft = run_with_pipe(ft_cmd)
        ls = run_with_pipe(ls_cmd)
    else:
        raise ValueError(f"unknown non-TTY mode: {mode}")

    diff = diff_output(ft.stdout, ls.stdout)
    if diff is not None:
        return TestFailure(None, ft_cmd, "stdout", diff, mode)

    diff = diff_output(ft.stderr, ls.stderr, normalize_program=True)
    if diff is not None:
        return TestFailure(None, ft_cmd, "stderr", diff, mode)

    if ft.returncode != ls.returncode:
        return TestFailure(
            None,
            ft_cmd,
            "returncode",
            f"ft_ls returncode: {ft.returncode}\nls returncode: {ls.returncode}",
            mode,
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
    cmd = shlex.join(failure.cmd)
    if failure.mode == "redirect":
        cmd = f"{cmd} > <tmpfile>"
    elif failure.mode == "pipe":
        cmd = f"{cmd} | <pipe>"

    lines = [
        "=" * 60,
        "Test failed",
        f"MODE: {failure.mode}",
    ]
    if failure.term_size is not None:
        lines.append(f"TERM_SIZE: {failure.term_size}")

    lines.extend(
        [
            f"CMD: {cmd}",
            f"KIND: {failure.kind}",
            "DIFF:",
            failure.diff.rstrip("\n"),
            "=" * 60,
        ]
    )

    return "\n".join(lines)


def run_with_redirect(cmd: list[str]) -> PtyResult:
    with tempfile.TemporaryFile() as stdout_file:
        result = subprocess.run(
            cmd,
            stdout=stdout_file,
            stderr=subprocess.PIPE,
            stdin=subprocess.DEVNULL,
            env=test_env(pty_mode=False),
        )
        stdout_file.seek(0)
        stdout = decode_bytes(stdout_file.read())

    return PtyResult(
        stdout=stdout,
        stderr=decode_bytes(result.stderr),
        returncode=result.returncode,
    )


def run_with_pipe(cmd: list[str]) -> PtyResult:
    result = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.DEVNULL,
        env=test_env(pty_mode=False),
    )

    return PtyResult(
        stdout=decode_bytes(result.stdout),
        stderr=decode_bytes(result.stderr),
        returncode=result.returncode,
    )


def run_with_pty(cmd: list[str], cols: int = 80) -> PtyResult:
    """Run command in a PTY with specified terminal width."""
    stdout_master, stdout_slave = pty.openpty()
    stderr_master, stderr_slave = pty.openpty()

    winsize = struct.pack("HHHH", 24, cols, 0, 0)
    fcntl.ioctl(stdout_slave, termios.TIOCSWINSZ, winsize)
    fcntl.ioctl(stderr_slave, termios.TIOCSWINSZ, winsize)
    env = test_env(pty_mode=True)

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


def test_env(pty_mode: bool) -> dict[str, str]:
    env = os.environ.copy()
    env["LC_ALL"] = "C"
    env["LANG"] = "C"
    env.pop("COLUMNS", None)
    if pty_mode:
        env["TERM"] = "screen-256color"

    return env


def decode_bytes(data: bytes) -> str:
    return data.decode(errors="surrogateescape")


if __name__ == "__main__":
    main()
