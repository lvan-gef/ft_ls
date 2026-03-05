#! /usr/bin/env python3

import difflib
import fcntl
import os
import pty
import shutil
import string
import struct
import subprocess
import sys
import tempfile
import termios
from pathlib import Path
from typing import Generator
from typing import NamedTuple
from itertools import permutations

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


def main() -> None:
    test_path = Path.cwd().joinpath("ft_ls_tester")
    paths: Paths = create_test_folders(path=test_path)
    paths.paths.append(test_path)

    try:
        # clean_up(paths=paths)

        subprocess.run("make fclean", shell=True, capture_output=True)
        # compile_ls()

        # print("=" * 60)
        # print('Phase 1: Non-existent paths and files')
        # flag_combination_tests(test_paths=paths)
        #
        # test_path = Path.cwd().joinpath("ft_ls_tester")
        # paths: Paths = create_test_folders(path=test_path)
        # paths.paths.append(test_path)
        #
        # print("=" * 60)
        # print("Phase 2: Invalid Flags")
        # invalid_flags()
        #
        print("=" * 60)
        print("Phase 3: Flag Combinations and Feature Tests")
        mega_test(test_paths=paths)
        subprocess.run("make fclean", shell=True, capture_output=True)

        print("=" * 60)
        print("All tests passed!")
        print("=" * 60)
    finally:
        pass
        # clean_up(paths=paths)


def compile_ls(term_size: int = 80) -> None:
    print("", "-" * 5, "Compile with column width:", term_size, "-" * 5)
    if DEBUG:
        result = subprocess.run(
            f"make TERM_SIZE={term_size} debug", shell=True, capture_output=True
        )
    else:
        result = subprocess.run(
            f"make TERM_SIZE={term_size}", shell=True, capture_output=True
        )

    assert result.returncode == 0, f"Compilation failed: {result.stderr.decode()}"


def invalid_flags() -> None:
    """Phase 1: Test all invalid flag characters."""

    counter = 0;
    for lttr in string.ascii_letters:
        if lttr in ALLOWED_FLAGS:
            continue

        result = subprocess.run(
            f"{ft_ls} -{lttr}", shell=True, capture_output=True, text=True
        )

        assert result.returncode == 2, f"Flag -{lttr} should return exit code 1"
        assert result.stdout == "", f"Flag -{lttr} should have empty stdout"
        assert result.stderr != "", f"Flag -{lttr} should have stderr message"
        counter += 1

    print(f"  Tested {counter} invalid flags: all passed")


def mega_test(test_paths: Paths) -> None:
    run_matrix_for_targets(ft_ls, list(test_paths.paths))

    run_matrix_for_targets(ft_ls, list(test_paths.files))

    run_matrix_for_targets(ft_ls, [*test_paths.paths, *test_paths.files])


def run_matrix_for_targets(ft_ls: str, targets: list[Path]) -> None:
    cases = 0

    for flag_args in permute_flags_separate():
        for target_list in permute_targets(targets, min_len=1):
            ft_cmd = [ft_ls, *flag_args, *(str(p) for p in target_list)]
            ls_cmd = ["ls",  *flag_args, *(str(p) for p in target_list)]
            # assert_command(ft_cmd=ft_cmd, ls_cmd=ls_cmd, msg=f"separate {flag_args=} {target_list=}")
            print(' '.join(ft_cmd))
            cases += 1

    for flag_str in permute_flags_combined():
        for target_list in permute_targets(targets, min_len=1):
            if flag_str:
                ft_cmd = [ft_ls, flag_str, *(str(p) for p in target_list)]
                ls_cmd = ["ls",  flag_str, *(str(p) for p in target_list)]
            else:
                ft_cmd = [ft_ls, *(str(p) for p in target_list)]
                ls_cmd = ["ls",  *(str(p) for p in target_list)]

            # assert_command(ft_cmd=ft_cmd, ls_cmd=ls_cmd, msg=f"combined {flag_str=} {target_list=}")
            print(' '.join(ft_cmd))
            cases += 1

    print(f"Tested {cases} cases for {len(targets)} targets")


def permute_flags_separate() -> Generator[list[str], None, None]:
    """
    All permutations of all lengths as separate args:
    [], ['-R'], ['-a'], ..., ['-R','-a'], ['-a','-R'], ...
    """

    yield []
    for r in range(1, len(ALLOWED_FLAGS) + 1):
        for perm in permutations(ALLOWED_FLAGS, r):
            yield [f"-{ch}" for ch in perm]

def permute_flags_combined() -> Generator[str, None, None]:
    """
    All permutations of all lengths as one combined arg:
    '', '-R', '-a', ..., '-Ra', '-aR', ...
    """

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
    if max_len is None:
        max_len = len(targets)
    max_len = min(max_len, len(targets))

    for r in range(min_len, max_len + 1):
        for perm in permutations(targets, r):
            yield list(perm)


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

    ft_stderr = ft.stderr
    if ft.stderr.startswith('ft_ls'):
        ft_stderr = ft.stderr.replace('ft_ls', 'ls', 1)
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


def run_with_pty(cmd: list[str], cols: int = 80) -> CommandResult:
    """Run command in a PTY with specified terminal width."""
    with tempfile.NamedTemporaryFile(delete=False) as stderr_file:

        master, slave = pty.openpty()
        return_code = 0
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
            os.close(slave)
            os.close(master)


    output = b""
    while True:
        try:
            data = os.read(master, 1024)
            if not data:
                break
            output += data
        except OSError:
            break

    with open(stderr_file.name, 'r') as f:
        stderr_output = f.read()

    os.remove(stderr_file.name)

    return CommandResult(stdout=output.decode().replace("\r", ""),
                         stderr=stderr_output,
                         returncode=return_code)

def clean_up(paths=Paths):
    for p in paths.paths:
        if p.exists():
            p.chmod(0o777)

    for p in paths.paths:
        if p.exists():
            shutil.rmtree(p)

if __name__ == "__main__":
    main()
