#! /usr/bin/env python3

import difflib
import fcntl
import os
import pty
import selectors
import shutil
import string
import struct
import subprocess
import sys
import termios
from pathlib import Path
from typing import NamedTuple

from create_test_folders import create_test_folders
from gen_data import ALLOWED_FLAGS, gen_data


class PtyResult(NamedTuple):
    stdout: str
    stderr: str
    returncode: int


DEBUG = True
TERMINAL_MIN = 40  # include
TERMINAL_MAX = 120  # exclude
own_bin = "./ft_ls"
if DEBUG:
    own_bin = f"{own_bin}_d"


def main() -> None:
    test_path = Path.cwd().joinpath("ft_ls_tester")
    if test_path.exists():
        remove_test_root(test_path)

    subprocess.run("make fclean", shell=True, capture_output=True)
    compile_ls(term_size=80)
    print("=" * 60)
    print("Phase 1: Invalid Flags")
    print("=" * 60)
    invalid_flags()

    try:
        data = create_test_folders(path=test_path)
        for term_size in range(TERMINAL_MIN, TERMINAL_MAX, 5):
            subprocess.run("make fclean", shell=True, capture_output=True)
            compile_ls(term_size=term_size)
            print("=" * 60)
            print("Phase 2-5: Flag Combinations and Feature Tests")
            print("=" * 60)
            for args in gen_data(paths=data):
                assert_output_match(
                    ft_cmd=[own_bin, *args],
                    ls_cmd=["ls", *args],
                    tern_size=term_size,
                )
    except Exception as e:
        print(e, file=sys.stderr)
        sys.exit(1)
    finally:
        remove_test_root(test_path)

    print("=" * 60)
    print("All tests passed!")
    print("=" * 60)


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


def compile_ls(term_size: int) -> None:
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
    for lttr in string.ascii_letters:
        if lttr in ALLOWED_FLAGS:
            continue

        result = subprocess.run(
            f"{own_bin} -{lttr}", shell=True, capture_output=True, text=True
        )
        assert result.returncode == 1, f"Flag -{lttr} should return exit code 1"
        assert result.stdout == "", f"Flag -{lttr} should have empty stdout"
        assert result.stderr != "", f"Flag -{lttr} should have stderr message"

    print(f"  Tested {52} invalid flags: all passed")


def assert_output_match(ft_cmd: list[str], ls_cmd: list[str], tern_size: int) -> None:
    """Assert ft_ls output matches ls output, showing diff on failure."""
    ft = run_with_pty(cmd=ft_cmd, cols=tern_size)
    ls = run_with_pty(cmd=ls_cmd, cols=tern_size)

    try:
        output_differ(
            ft_out=ft.stdout, ls_out=ls.stdout, cmd=ft_cmd, kind_output="stdout"
        )
    except AssertionError as ae:
        raise (ae)

    try:
        output_differ(
            ft_out=ft.stderr, ls_out=ls.stderr, cmd=ft_cmd, kind_output="stderr"
        )
    except AssertionError as ae:
        raise (ae)
    try:
        assert ft.returncode == ls.returncode
    except AssertionError:
        print(
            f"ft_ls returncode: {ft.returncode}\nls returncode: {ls.returncode}",
            file=sys.stderr,
        )
        raise AssertionError(f"Returncode mismatch for: {' '.join(ft_cmd)}")


def normalize_program_name(output: str) -> str:
    lines: list[str] = []

    for line in output.splitlines(keepends=True):
        if line.startswith("ft_ls_d"):
            lines.append(line.replace("ft_ls_d", "ls", 1))
        elif line.startswith("ft_ls"):
            lines.append(line.replace("ft_ls", "ls", 1))
        else:
            lines.append(line)

    return "".join(lines)


def output_differ(ft_out: str, ls_out: str, cmd: list[str], kind_output: str) -> None:
    ft_out_cpy = normalize_program_name(ft_out)
    try:
        assert ft_out_cpy == ls_out
    except AssertionError:
        seqm = difflib.SequenceMatcher(None, ft_out_cpy, ls_out)
        for opcode, a0, a1, b0, b1 in seqm.get_opcodes():
            if opcode == "replace":
                print(
                    f"Replace:\n'{ft_out_cpy[a0:a1]}'\nWith:\n'{ls_out[b0:b1]}'",
                    file=sys.stderr,
                )
            elif opcode == "insert":
                print(f"Insert:\n'{ls_out[b0:b1]}'", file=sys.stderr)
            elif opcode == "delete":
                print(f"Delete:\n'{ft_out_cpy[a0:a1]}'", file=sys.stderr)
        raise AssertionError(f"Output {kind_output} mismatch for: {' '.join(cmd)}")
    pass


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
    chunks: dict[str, list[bytes]] = {"stdout": [], "stderr": []}

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

            chunks[key.data].append(data)

    returncode = proc.wait()
    selector.close()

    return PtyResult(
        stdout=b"".join(chunks["stdout"]).decode().replace("\r", ""),
        stderr=b"".join(chunks["stderr"]).decode().replace("\r", ""),
        returncode=returncode,
    )


if __name__ == "__main__":
    main()
