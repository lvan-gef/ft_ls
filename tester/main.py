#! /usr/bin/env python3

import argparse
import difflib
import fcntl
import multiprocessing as mp
import os
import pty
import shutil
import string
import struct
import subprocess
import sys
import tempfile
import termios
from dataclasses import dataclass
from itertools import permutations
from pathlib import Path
from typing import Generator, NamedTuple

from create_test_folders import create_test_folders, Paths


class CommandResult(NamedTuple):
    stdout: str
    stderr: str
    returncode: int


ALLOWED_FLAGS = ["R", "a", "l", "r", "t"]

DEBUG = True
TERMINAL_MIN = 80  # include
TERMINAL_MAX = 513  # exclude

ft_ls = "./ft_ls"
if DEBUG:
    ft_ls = f"{ft_ls}_d"


# -----------------------------
# Flag + target permutations
# -----------------------------
def permute_flags_separate(*, include_empty: bool = True) -> Generator[list[str], None, None]:
    """
    All permutations of all lengths as separate args:
    [], ['-R'], ['-a'], ..., ['-R','-a'], ['-a','-R'], ...
    """
    if include_empty:
        yield []
    for r in range(1, len(ALLOWED_FLAGS) + 1):
        for perm in permutations(ALLOWED_FLAGS, r):
            yield [f"-{ch}" for ch in perm]


def permute_flags_combined(*, include_empty: bool = True) -> Generator[str, None, None]:
    """
    All permutations of all lengths as one combined arg:
    '', '-R', '-a', ..., '-Ra', '-aR', ...
    """
    if include_empty:
        yield ""
    for r in range(1, len(ALLOWED_FLAGS) + 1):
        for perm in permutations(ALLOWED_FLAGS, r):
            yield "-" + "".join(perm)


def permute_targets(
    targets: list[Path],
    *,
    min_len: int = 1,
    max_len: int | None = None,
) -> Generator[list[Path], None, None]:
    """
    Permutations of the *targets themselves* (because ls can take multiple paths):
    [p1], [p2], [p1,p2], [p2,p1], ...
    """
    if not targets:
        return
    if max_len is None:
        max_len = len(targets)
    max_len = max(0, min(max_len, len(targets)))
    min_len = max(0, min(min_len, max_len))

    for r in range(min_len, max_len + 1):
        for perm in permutations(targets, r):
            yield list(perm)


# -----------------------------
# Running + comparing commands
# -----------------------------
def run_with_pty(cmd: list[str], cols: int = 80) -> CommandResult:
    """Run command in a PTY with specified terminal width."""
    with tempfile.NamedTemporaryFile(delete=False) as stderr_file:
        master, slave = pty.openpty()
        try:
            winsize = struct.pack("HHHH", 24, cols, 0, 0)  # rows, cols, xpixel, ypixel
            fcntl.ioctl(slave, termios.TIOCSWINSZ, winsize)

            env = os.environ.copy()
            env["LC_ALL"] = "C"
            env["LANG"] = "C"
            env.pop("COLUMNS", None)
            env["TERM"] = "screen-256color"

            proc = subprocess.Popen(
                cmd,
                stdout=slave,
                stderr=stderr_file,
                stdin=slave,
                close_fds=True,
                env=env,
            )
            proc.wait()
            return_code = proc.returncode
        finally:
            # close slave first so reads on master drain properly
            try:
                os.close(slave)
            except OSError:
                pass

        # Read from master BEFORE closing it
        output = b""
        try:
            while True:
                data = os.read(master, 4096)
                if not data:
                    break
                output += data
        except OSError:
            pass
        finally:
            try:
                os.close(master)
            except OSError:
                pass

    with open(stderr_file.name, "r", encoding="utf-8", errors="replace") as f:
        stderr_output = f.read()
    os.remove(stderr_file.name)

    return CommandResult(
        stdout=output.decode(errors="replace").replace("\r", ""),
        stderr=stderr_output,
        returncode=return_code,
    )


def assert_command(ft_cmd: list[str], ls_cmd: list[str], msg: str) -> None:
    """Assert ft_ls output matches ls output, showing diff on failure."""
    ft: CommandResult = run_with_pty(cmd=ft_cmd)
    ls: CommandResult = run_with_pty(cmd=ls_cmd)

    if ft.stdout != ls.stdout:
        print(f"\nMismatch on stdout for: {' '.join(ft_cmd)}", file=sys.stderr)
        print(f"ls stdout   :\n{ls.stdout}", file=sys.stderr)
        print("-" * 40, file=sys.stderr)
        print(f"ft_ls stdout:\n{ft.stdout}", file=sys.stderr)

        seqm = difflib.SequenceMatcher(None, ft.stdout, ls.stdout)
        for opcode, a0, a1, b0, b1 in seqm.get_opcodes():
            if opcode == "replace":
                print(
                    f"Replace:\n'{ft.stdout[a0:a1]}'\nWith:\n'{ls.stdout[b0:b1]}'",
                    file=sys.stderr,
                )
            elif opcode == "insert":
                print(f"Insert:\n'{ls.stdout[b0:b1]}'", file=sys.stderr)
            elif opcode == "delete":
                print(f"Delete:\n'{ft.stdout[a0:a1]}'", file=sys.stderr)

        raise AssertionError(f"{msg}: {' '.join(ft_cmd)}")

    # Normalize your program name in stderr if needed
    ft_stderr = ft.stderr
    if ft_stderr.startswith("ft_ls"):
        ft_stderr = ft_stderr.replace("ft_ls", "ls", 1)

    if ft_stderr != ls.stderr:
        print(f"\nMismatch on stderr for: {' '.join(ft_cmd)}", file=sys.stderr)
        print(f"ls stderr   :\n{ls.stderr}", file=sys.stderr)
        print("-" * 40, file=sys.stderr)
        print(f"ft_ls stderr:\n{ft_stderr}", file=sys.stderr)

        seqm = difflib.SequenceMatcher(None, ft_stderr, ls.stderr)
        for opcode, a0, a1, b0, b1 in seqm.get_opcodes():
            if opcode == "replace":
                print(
                    f"Replace:\n'{ft_stderr[a0:a1]}'\nWith:\n'{ls.stderr[b0:b1]}'",
                    file=sys.stderr,
                )
            elif opcode == "insert":
                print(f"Insert:\n'{ls.stderr[b0:b1]}'", file=sys.stderr)
            elif opcode == "delete":
                print(f"Delete:\n'{ft_stderr[a0:a1]}'", file=sys.stderr)

        raise AssertionError(f"{msg}: {' '.join(ft_cmd)}")

    if ft.returncode != ls.returncode:
        print(f"\nMismatch on returncode for: {' '.join(ft_cmd)}", file=sys.stderr)
        print(f"ls returncode   : {ls.returncode}", file=sys.stderr)
        print("-" * 40, file=sys.stderr)
        print(f"ft_ls returncode: {ft.returncode}", file=sys.stderr)
        raise AssertionError(f"{msg}: {' '.join(ft_cmd)}")


# -----------------------------
# Test phases
# -----------------------------
def compile_ls(term_size: int = 80) -> None:
    print("", "-" * 5, "Compile with column width:", term_size, "-" * 5)
    if DEBUG:
        result = subprocess.run(f"make TERM_SIZE={term_size} debug", shell=True, capture_output=True)
    else:
        result = subprocess.run(f"make TERM_SIZE={term_size}", shell=True, capture_output=True)

    assert result.returncode == 0, f"Compilation failed: {result.stderr.decode(errors='replace')}"


def invalid_flags() -> None:
    """Test all invalid flag characters."""
    counter = 0
    for lttr in string.ascii_letters:
        if lttr in ALLOWED_FLAGS:
            continue

        result = subprocess.run(f"{ft_ls} -{lttr}", shell=True, capture_output=True, text=True)

        assert result.returncode == 2, f"Flag -{lttr} should return exit code 2"
        assert result.stdout == "", f"Flag -{lttr} should have empty stdout"
        assert result.stderr != "", f"Flag -{lttr} should have stderr message"
        counter += 1

    print(f"  Tested {counter} invalid flags: all passed")


def run_matrix_for_targets(
    targets: list[Path],
    *,
    max_target_len: int | None = None,
    include_empty_flags: bool = True,
    run_separate_flags: bool = True,
    run_combined_flags: bool = True,
) -> None:
    """
    Run: (flags permutations) x (target permutations).
    Raises on first mismatch (good for debugging). For logging-all-mismatches, use multiprocessing mode below.
    """
    cases = 0

    if run_separate_flags:
        for flag_args in permute_flags_separate(include_empty=include_empty_flags):
            for target_list in permute_targets(targets, min_len=1, max_len=max_target_len):
                ft_cmd = [ft_ls, *flag_args, *(str(p) for p in target_list)]
                ls_cmd = ["ls", *flag_args, *(str(p) for p in target_list)]
                assert_command(ft_cmd=ft_cmd, ls_cmd=ls_cmd, msg="separate flags")
                cases += 1

    if run_combined_flags:
        for flag_str in permute_flags_combined(include_empty=include_empty_flags):
            for target_list in permute_targets(targets, min_len=1, max_len=max_target_len):
                if flag_str:
                    ft_cmd = [ft_ls, flag_str, *(str(p) for p in target_list)]
                    ls_cmd = ["ls", flag_str, *(str(p) for p in target_list)]
                else:
                    ft_cmd = [ft_ls, *(str(p) for p in target_list)]
                    ls_cmd = ["ls", *(str(p) for p in target_list)]
                assert_command(ft_cmd=ft_cmd, ls_cmd=ls_cmd, msg="combined flags")
                cases += 1

    print(f"Tested {cases} cases for {len(targets)} targets")


def mega_test(
    test_paths: Paths,
    *,
    max_target_len: int | None = None,
    include_empty_flags: bool = True,
    run_separate_flags: bool = True,
    run_combined_flags: bool = True,
) -> None:
    run_matrix_for_targets(
        list(test_paths.paths),
        max_target_len=max_target_len,
        include_empty_flags=include_empty_flags,
        run_separate_flags=run_separate_flags,
        run_combined_flags=run_combined_flags,
    )
    run_matrix_for_targets(
        list(test_paths.files),
        max_target_len=max_target_len,
        include_empty_flags=include_empty_flags,
        run_separate_flags=run_separate_flags,
        run_combined_flags=run_combined_flags,
    )
    run_matrix_for_targets(
        [*test_paths.paths, *test_paths.files],
        max_target_len=max_target_len,
        include_empty_flags=include_empty_flags,
        run_separate_flags=run_separate_flags,
        run_combined_flags=run_combined_flags,
    )


# -----------------------------
# Multiprocessing (3 phases in parallel) with per-phase logs
# -----------------------------
@dataclass(frozen=True)
class PhaseJob:
    name: str
    targets: list[Path]
    log_path: Path
    max_target_len: int | None
    include_empty_flags: bool
    run_separate_flags: bool
    run_combined_flags: bool


def _run_phase(job: PhaseJob) -> int:
    """
    Worker: run one phase, collect ALL mismatches into a log file.
    Returns: 0 OK, 1 failures, 2 crash.
    """
    failures = 0
    total = 0

    try:
        with open(job.log_path, "w", encoding="utf-8") as log:
            log.write(f"== Phase {job.name} ==\n")
            log.write(f"targets={len(job.targets)} max_target_len={job.max_target_len}\n")
            log.flush()

            def record_fail(ft_cmd: list[str], err: Exception) -> None:
                nonlocal failures
                failures += 1
                log.write(f"\nFAIL #{failures}\nCMD: {' '.join(ft_cmd)}\nERR: {err}\n")
                log.flush()

            if job.run_separate_flags:
                for flag_args in permute_flags_separate(include_empty=job.include_empty_flags):
                    for target_list in permute_targets(job.targets, min_len=1, max_len=job.max_target_len):
                        ft_cmd = [ft_ls, *flag_args, *(str(p) for p in target_list)]
                        ls_cmd = ["ls", *flag_args, *(str(p) for p in target_list)]
                        total += 1
                        try:
                            assert_command(ft_cmd=ft_cmd, ls_cmd=ls_cmd, msg=f"{job.name}: separate")
                        except AssertionError as e:
                            record_fail(ft_cmd, e)

            if job.run_combined_flags:
                for flag_str in permute_flags_combined(include_empty=job.include_empty_flags):
                    for target_list in permute_targets(job.targets, min_len=1, max_len=job.max_target_len):
                        if flag_str:
                            ft_cmd = [ft_ls, flag_str, *(str(p) for p in target_list)]
                            ls_cmd = ["ls", flag_str, *(str(p) for p in target_list)]
                        else:
                            ft_cmd = [ft_ls, *(str(p) for p in target_list)]
                            ls_cmd = ["ls", *(str(p) for p in target_list)]
                        total += 1
                        try:
                            assert_command(ft_cmd=ft_cmd, ls_cmd=ls_cmd, msg=f"{job.name}: combined")
                        except AssertionError as e:
                            record_fail(ft_cmd, e)

            log.write(f"\nDone. total_cases={total} failures={failures}\n")

        return 0 if failures == 0 else 1

    except Exception as e:
        try:
            with open(job.log_path, "a", encoding="utf-8") as log:
                log.write(f"\nCRASH: {repr(e)}\n")
        except Exception:
            pass
        return 2


# -----------------------------
# Cleanup
# -----------------------------
def clean_up(paths: Paths) -> None:
    # restore perms (important if you made 000 dirs)
    for p in paths.paths:
        if p.exists():
            try:
                p.chmod(0o777)
            except Exception:
                pass

    # delete tree roots
    for p in paths.paths:
        if p.exists():
            try:
                shutil.rmtree(p)
            except Exception:
                pass


# -----------------------------
# CLI / main
# -----------------------------
def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--phase",
        choices=["dirs", "files", "both", "all", "invalid"],
        default="all",
        help="Which target set to test.",
    )
    parser.add_argument(
        "--parallel",
        action="store_true",
        help="Run dirs/files/both phases in parallel (multiprocessing) and write logs per phase.",
    )
    parser.add_argument(
        "--max-target-len",
        type=int,
        default=None,
        help="Limit permutation length for target lists (VERY recommended). Example: 2 or 3.",
    )
    parser.add_argument(
        "--no-empty-flags",
        action="store_true",
        help="Do not include the empty flag case (no flags).",
    )
    parser.add_argument(
        "--flags",
        choices=["separate", "combined", "both"],
        default="both",
        help="Which flag style(s) to test.",
    )
    parser.add_argument(
        "--no-clean",
        action="store_true",
        help="Do not delete the created ft_ls_tester folder at the end.",
    )
    args = parser.parse_args()

    include_empty_flags = not args.no_empty_flags
    run_separate = args.flags in ("separate", "both")
    run_combined = args.flags in ("combined", "both")

    test_root = Path.cwd().joinpath("ft_ls_tester")
    paths: Paths = create_test_folders(path=test_root)
    paths.paths.append(test_root)

    try:
        subprocess.run("make fclean", shell=True, capture_output=True)
        subprocess.run("make debug", shell=True)

        if args.phase == "invalid":
            invalid_flags()
            return

        if args.parallel:
            logs_dir = Path("tester_logs")
            logs_dir.mkdir(exist_ok=True)

            jobs = [
                PhaseJob(
                    "dirs",
                    list(paths.paths),
                    logs_dir / "dirs.log",
                    args.max_target_len,
                    include_empty_flags,
                    run_separate,
                    run_combined,
                ),
                PhaseJob(
                    "files",
                    list(paths.files),
                    logs_dir / "files.log",
                    args.max_target_len,
                    include_empty_flags,
                    run_separate,
                    run_combined,
                ),
                PhaseJob(
                    "both",
                    [*paths.paths, *paths.files],
                    logs_dir / "both.log",
                    args.max_target_len,
                    include_empty_flags,
                    run_separate,
                    run_combined,
                ),
            ]

            # spawn is safer cross-platform; fine on Linux too
            ctx = mp.get_context("spawn")
            with ctx.Pool(processes=min(3, os.cpu_count() or 3)) as pool:
                results = pool.map(_run_phase, jobs)

            for job, rc in zip(jobs, results):
                status = "OK" if rc == 0 else ("FAIL" if rc == 1 else "CRASH")
                print(f"[{status}] {job.name} -> {job.log_path}")

            if any(rc != 0 for rc in results):
                raise SystemExit(1)

        else:
            if args.phase == "dirs":
                run_matrix_for_targets(
                    list(paths.paths),
                    max_target_len=args.max_target_len,
                    include_empty_flags=include_empty_flags,
                    run_separate_flags=run_separate,
                    run_combined_flags=run_combined,
                )
            elif args.phase == "files":
                run_matrix_for_targets(
                    list(paths.files),
                    max_target_len=args.max_target_len,
                    include_empty_flags=include_empty_flags,
                    run_separate_flags=run_separate,
                    run_combined_flags=run_combined,
                )
            elif args.phase == "both":
                run_matrix_for_targets(
                    [*paths.paths, *paths.files],
                    max_target_len=args.max_target_len,
                    include_empty_flags=include_empty_flags,
                    run_separate_flags=run_separate,
                    run_combined_flags=run_combined,
                )
            else:  # all
                mega_test(
                    test_paths=paths,
                    max_target_len=args.max_target_len,
                    include_empty_flags=include_empty_flags,
                    run_separate_flags=run_separate,
                    run_combined_flags=run_combined,
                )

        subprocess.run("make fclean", shell=True, capture_output=True)

        print("=" * 60)
        print("All requested tests passed!")
        print("=" * 60)

    finally:
        if not args.no_clean:
            clean_up(paths=paths)


if __name__ == "__main__":
    main()
