#!/usr/bin/env python3
from __future__ import annotations

import argparse
import difflib
import fcntl
import os
import pty
import shlex
import shutil
import statistics
import selectors
import subprocess
import struct
import sys
import termios
import time
from dataclasses import dataclass
from pathlib import Path


DEFAULT_ROOT = Path("ft_ls_compare_tree")
DEFAULT_RESULTS = Path("ft_ls_compare_results")
MARKER_FILE = ".compare_ls_tree"
BASE_MTIME = 1_704_067_200  # 2024-01-01 00:00:00 UTC


@dataclass(frozen=True)
class TreeStats:
    dirs: int
    files: int
    symlinks: int


@dataclass(frozen=True)
class CaptureResult:
    returncode: int
    stdout: Path
    stderr: Path


@dataclass(frozen=True)
class PtyResult:
    returncode: int
    stdout: bytes
    stderr: bytes


def positive_int(value: str) -> int:
    try:
        number = int(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"expected an integer, got {value!r}") from exc
    if number <= 0:
        raise argparse.ArgumentTypeError(f"expected a positive integer, got {number}")
    return number


def non_negative_int(value: str) -> int:
    try:
        number = int(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"expected an integer, got {value!r}") from exc
    if number < 0:
        raise argparse.ArgumentTypeError(f"expected a non-negative integer, got {number}")
    return number


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a recursive test tree, compare ls with ft_ls, and time both."
    )
    parser.add_argument(
        "--bin",
        dest="ft_bin",
        default=None,
        help="ft_ls binary to run (default: ./ft_ls_bonus if present, else ./ft_ls)",
    )
    parser.add_argument(
        "--ls-bin",
        default=None,
        help="system ls binary to run (default: first ls in PATH)",
    )
    parser.add_argument(
        "--flags",
        default="-Rl",
        help="flags passed to both binaries; use --flags=-Ral for leading dashes",
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=DEFAULT_ROOT,
        help=f"test tree path (default: {DEFAULT_ROOT})",
    )
    parser.add_argument(
        "--results",
        type=Path,
        default=DEFAULT_RESULTS,
        help=f"output directory for captured output and diffs (default: {DEFAULT_RESULTS})",
    )
    parser.add_argument(
        "--cols",
        type=positive_int,
        default=80,
        help="PTY terminal width used for both ls and ft_ls",
    )
    parser.add_argument(
        "--time-mode",
        choices=("devnull", "pty"),
        default="devnull",
        help="benchmark output mode; devnull avoids PTY overhead and matches timer.py",
    )
    parser.add_argument(
        "--wide-dirs",
        type=positive_int,
        default=1000,
        help="number of sibling directories in the wide stress case",
    )
    parser.add_argument(
        "--wide-files",
        type=positive_int,
        default=8,
        help="files created in each wide directory",
    )
    parser.add_argument(
        "--wide-nested-every",
        type=positive_int,
        default=100,
        help="add one nested directory every N wide directories",
    )
    parser.add_argument(
        "--deep-levels",
        type=positive_int,
        default=120,
        help="depth of the single recursive chain",
    )
    parser.add_argument(
        "--deep-file-step",
        type=positive_int,
        default=10,
        help="add one more file per deep directory every N levels",
    )
    parser.add_argument(
        "--max-deep-files",
        type=positive_int,
        default=32,
        help="cap for files created in one deep directory",
    )
    parser.add_argument(
        "--runs",
        type=positive_int,
        default=7,
        help="timed runs per binary",
    )
    parser.add_argument(
        "--warmup",
        type=non_negative_int,
        default=1,
        help="untimed warmup runs per binary before timing",
    )
    parser.add_argument(
        "--no-generate",
        action="store_true",
        help="reuse the existing tree instead of regenerating it",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="allow deleting --root even if it does not contain the script marker",
    )
    return parser.parse_args()


def choose_ft_bin(raw_bin: str | None) -> str:
    candidates = ("./ft_ls_bonus", "./ft_ls", "./ft_ls_bonus_d", "./ft_ls_d")
    if raw_bin is None:
        for candidate in candidates:
            path = Path(candidate)
            if path.is_file() and os.access(path, os.X_OK):
                return candidate
        raise RuntimeError(
            "could not find an executable ft_ls binary; run `make bonus` or pass --bin"
        )
    return resolve_executable(raw_bin, "ft_ls")


def choose_ls_bin(raw_bin: str | None) -> str:
    if raw_bin is not None:
        return resolve_executable(raw_bin, "ls")
    ls_bin = shutil.which("ls")
    if ls_bin is None:
        raise RuntimeError("could not find ls in PATH")
    return ls_bin


def resolve_executable(value: str, name: str) -> str:
    if "/" not in value:
        resolved = shutil.which(value)
        if resolved is None:
            raise RuntimeError(f"could not find {name} binary {value!r} in PATH")
        return resolved

    path = Path(value)
    if not path.is_file():
        raise RuntimeError(f"{name} binary does not exist: {value}")
    if not os.access(path, os.X_OK):
        raise RuntimeError(f"{name} binary is not executable: {value}")
    return value


def test_env() -> dict[str, str]:
    env = os.environ.copy()
    env["LC_ALL"] = "C"
    env["LANG"] = "C"
    env["TERM"] = "screen-256color"
    env["TZ"] = "UTC"
    env.pop("COLUMNS", None)
    for key in (
        "BLOCK_SIZE",
        "LS_BLOCK_SIZE",
        "POSIXLY_CORRECT",
        "QUOTING_STYLE",
        "TIME_STYLE",
    ):
        env.pop(key, None)
    return env


def recreate_root(root: Path, force: bool) -> None:
    if root.exists():
        marker = root / MARKER_FILE
        if not root.is_dir():
            raise RuntimeError(f"refusing to remove non-directory root: {root}")
        if not marker.exists() and not force:
            raise RuntimeError(
                f"refusing to remove {root}: marker {MARKER_FILE!r} is missing; "
                "use --force if this is intentional"
            )
        shutil.rmtree(root)

    root.mkdir(parents=True)
    marker = root / MARKER_FILE
    marker.write_text("created by compare_ls.py\n", encoding="utf-8")
    os.utime(marker, (BASE_MTIME, BASE_MTIME))


def generate_tree(args: argparse.Namespace) -> TreeStats:
    root = args.root.expanduser()
    recreate_root(root, args.force)

    dirs: set[Path] = {root}
    files: set[Path] = set()
    symlinks: set[Path] = set()

    def add_dir(path: Path) -> Path:
        path.mkdir()
        dirs.add(path)
        return path

    def write_file(path: Path, content: str, mode: int, mtime: int) -> None:
        path.write_text(content, encoding="utf-8")
        path.chmod(mode)
        os.utime(path, (mtime, mtime))
        files.add(path)

    def add_symlink(target: str, link: Path) -> None:
        link.symlink_to(target)
        try:
            os.utime(link, (BASE_MTIME, BASE_MTIME), follow_symlinks=False)
        except (NotImplementedError, OSError):
            pass
        symlinks.add(link)

    wide_root = add_dir(root / "wide")
    for dir_index in range(args.wide_dirs):
        directory = add_dir(wide_root / f"dir_{dir_index:04d}")
        for file_index in range(args.wide_files):
            mode = 0o755 if file_index % 5 == 0 else 0o644
            write_file(
                directory / f"file_{file_index:03d}.txt",
                f"wide dir {dir_index} file {file_index}\n",
                mode,
                BASE_MTIME + dir_index + file_index,
            )

        if dir_index % args.wide_nested_every == 0:
            nested = add_dir(directory / "nested")
            write_file(
                nested / "nested_file.txt",
                f"nested under wide dir {dir_index}\n",
                0o644,
                BASE_MTIME + dir_index,
            )

    deep_root = add_dir(root / "deep")
    current = deep_root
    for depth in range(args.deep_levels):
        current = add_dir(current / f"level_{depth:03d}")
        file_count = min(args.max_deep_files, 1 + depth // args.deep_file_step)
        for file_index in range(file_count):
            write_file(
                current / f"depth_{depth:03d}_file_{file_index:03d}.dat",
                f"depth {depth} file {file_index}\n",
                0o600 if file_index % 7 == 0 else 0o644,
                BASE_MTIME + depth * 60 + file_index,
            )

    special_root = add_dir(root / "special_names")
    for dirname in ("space dir", "colon:dir", "brackets[dir]", "quote'dir"):
        directory = add_dir(special_root / dirname)
        write_file(directory / "plain.txt", f"inside {dirname}\n", 0o644, BASE_MTIME)
    for filename in (
        "name with spaces.txt",
        "semi;colon.txt",
        "hash#file.txt",
        "quote'file.txt",
        "brackets[file].txt",
    ):
        write_file(special_root / filename, f"special {filename}\n", 0o644, BASE_MTIME)

    links_root = add_dir(root / "links")
    add_symlink("../wide/dir_0000/file_000.txt", links_root / "link_to_file")
    add_symlink("../wide/dir_0000", links_root / "link_to_dir")
    add_symlink("../missing_target", links_root / "broken_link")

    hard_root = add_dir(root / "hardlinks")
    hard_link = hard_root / "hardlink_to_file_000.txt"
    os.link(wide_root / "dir_0000" / "file_000.txt", hard_link)
    os.utime(hard_link, (BASE_MTIME, BASE_MTIME))
    files.add(hard_link)

    empty_root = add_dir(root / "empty_dirs")
    for index in range(10):
        add_dir(empty_root / f"empty_{index:02d}")

    for directory in sorted(dirs, key=lambda item: len(item.parts), reverse=True):
        os.utime(directory, (BASE_MTIME, BASE_MTIME))

    return TreeStats(dirs=len(dirs), files=len(files), symlinks=len(symlinks))


def split_flags(flags: str) -> list[str]:
    if not flags:
        return []
    return shlex.split(flags)


def command_string(cmd: list[str]) -> str:
    return " ".join(shlex.quote(part) for part in cmd)


def capture_command(
    cmd: list[str], stdout_path: Path, stderr_path: Path, cols: int,
    env: dict[str, str]
) -> CaptureResult:
    result = run_with_pty(cmd, cols, env)
    stdout_path.write_bytes(result.stdout)
    stderr_path.write_bytes(result.stderr)
    return CaptureResult(result.returncode, stdout_path, stderr_path)


def run_with_pty(cmd: list[str], cols: int, env: dict[str, str]) -> PtyResult:
    stdout_master, stdout_slave = pty.openpty()
    stderr_master, stderr_slave = pty.openpty()
    selector = selectors.DefaultSelector()
    proc: subprocess.Popen[bytes] | None = None

    try:
        winsize = struct.pack("HHHH", 24, cols, 0, 0)
        fcntl.ioctl(stdout_slave, termios.TIOCSWINSZ, winsize)
        fcntl.ioctl(stderr_slave, termios.TIOCSWINSZ, winsize)

        proc = subprocess.Popen(
            cmd,
            stdout=stdout_slave,
            stderr=stderr_slave,
            stdin=stdout_slave,
            close_fds=True,
            env=env,
        )
    finally:
        os.close(stdout_slave)
        os.close(stderr_slave)

    selector.register(stdout_master, selectors.EVENT_READ, "stdout")
    selector.register(stderr_master, selectors.EVENT_READ, "stderr")
    chunks: dict[str, list[bytes]] = {"stdout": [], "stderr": []}

    try:
        while selector.get_map():
            for key, _ in selector.select():
                try:
                    data = os.read(key.fd, 65536)
                except OSError:
                    data = b""

                if not data:
                    selector.unregister(key.fd)
                    os.close(key.fd)
                    continue

                chunks[key.data].append(data)
    finally:
        for key in list(selector.get_map().values()):
            selector.unregister(key.fd)
            os.close(key.fd)
        selector.close()

    assert proc is not None
    returncode = proc.wait()
    return PtyResult(
        returncode=returncode,
        stdout=b"".join(chunks["stdout"]).replace(b"\r", b""),
        stderr=b"".join(chunks["stderr"]).replace(b"\r", b""),
    )


def write_diff(left: Path, right: Path, diff_path: Path) -> bool:
    diff_bin = shutil.which("diff")
    if diff_bin is not None:
        with diff_path.open("wb") as output:
            result = subprocess.run(
                [diff_bin, "-u", str(left), str(right)],
                stdout=output,
                stderr=subprocess.PIPE,
                check=False,
            )
        if result.returncode > 1:
            raise RuntimeError(result.stderr.decode("utf-8", errors="replace"))
        return result.returncode == 0

    left_lines = left.read_text(encoding="utf-8", errors="replace").splitlines(True)
    right_lines = right.read_text(encoding="utf-8", errors="replace").splitlines(True)
    diff = difflib.unified_diff(
        left_lines,
        right_lines,
        fromfile=str(left),
        tofile=str(right),
    )
    diff_text = "".join(diff)
    diff_path.write_text(diff_text, encoding="utf-8")
    return diff_text == ""


def run_once(cmd: list[str], cols: int, time_mode: str,
             env: dict[str, str]) -> float:
    start = time.perf_counter()
    if time_mode == "pty":
        returncode = run_with_pty(cmd, cols, env).returncode
    else:
        returncode = subprocess.run(
            cmd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            env=env,
            check=False,
        ).returncode
    duration = time.perf_counter() - start
    if returncode != 0:
        raise RuntimeError(
            f"command exited with status {returncode}: {command_string(cmd)}"
        )
    return duration


def benchmark(
    ls_cmd: list[str], ft_cmd: list[str], runs: int, warmup: int, cols: int,
    time_mode: str, env: dict[str, str]
) -> tuple[list[float], list[float]]:
    for _ in range(warmup):
        run_once(ls_cmd, cols, time_mode, env)
        run_once(ft_cmd, cols, time_mode, env)

    ls_durations: list[float] = []
    ft_durations: list[float] = []
    for index in range(runs):
        if index % 2 == 0:
            ls_durations.append(run_once(ls_cmd, cols, time_mode, env))
            ft_durations.append(run_once(ft_cmd, cols, time_mode, env))
        else:
            ft_durations.append(run_once(ft_cmd, cols, time_mode, env))
            ls_durations.append(run_once(ls_cmd, cols, time_mode, env))
    return ls_durations, ft_durations


def timing_line(name: str, durations: list[float]) -> str:
    return (
        f"{name}: median={statistics.median(durations):.6f}s "
        f"avg={statistics.mean(durations):.6f}s "
        f"min={min(durations):.6f}s max={max(durations):.6f}s"
    )


def write_summary(results_dir: Path, lines: list[str]) -> None:
    summary_path = results_dir / "summary.txt"
    summary_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    args.root = args.root.expanduser()
    args.results = args.results.expanduser()

    try:
        ls_bin = choose_ls_bin(args.ls_bin)
        ft_bin = choose_ft_bin(args.ft_bin)
        flags = split_flags(args.flags)
        env = test_env()

        stats: TreeStats | None = None
        if args.no_generate:
            if not args.root.is_dir():
                raise RuntimeError(f"tree does not exist: {args.root}")
        else:
            stats = generate_tree(args)

        args.results.mkdir(parents=True, exist_ok=True)
        ls_cmd = [ls_bin, *flags, str(args.root)]
        ft_cmd = [ft_bin, *flags, str(args.root)]

        ls_result = capture_command(
            ls_cmd,
            args.results / "ls.stdout",
            args.results / "ls.stderr",
            args.cols,
            env,
        )
        ft_result = capture_command(
            ft_cmd,
            args.results / "ft_ls.stdout",
            args.results / "ft_ls.stderr",
            args.cols,
            env,
        )

        stdout_same = write_diff(
            ls_result.stdout,
            ft_result.stdout,
            args.results / "stdout.diff",
        )
        stderr_same = write_diff(
            ls_result.stderr,
            ft_result.stderr,
            args.results / "stderr.diff",
        )
        returncodes_same = ls_result.returncode == ft_result.returncode

        ls_times: list[float] = []
        ft_times: list[float] = []
        if ls_result.returncode == 0 and ft_result.returncode == 0:
            ls_times, ft_times = benchmark(
                ls_cmd,
                ft_cmd,
                args.runs,
                args.warmup,
                args.cols,
                args.time_mode,
                env,
            )

        lines: list[str] = []
        if stats is not None:
            lines.append(
                f"tree: {args.root} ({stats.dirs} dirs, {stats.files} files, "
                f"{stats.symlinks} symlinks)"
            )
        else:
            lines.append(f"tree: {args.root} (reused)")
        lines.append(f"ls:    {command_string(ls_cmd)}")
        lines.append(f"ft_ls: {command_string(ft_cmd)}")
        lines.append("compare mode: pty")
        lines.append(f"timing mode: {args.time_mode}")
        lines.append(f"pty cols: {args.cols}")
        lines.append(f"ls returncode:    {ls_result.returncode}")
        lines.append(f"ft_ls returncode: {ft_result.returncode}")
        lines.append(f"stdout: {'match' if stdout_same else 'DIFFER'}")
        lines.append(f"stderr: {'match' if stderr_same else 'DIFFER'}")
        lines.append(f"outputs: {args.results}")

        if ls_times and ft_times:
            ls_median = statistics.median(ls_times)
            ft_median = statistics.median(ft_times)
            lines.append(timing_line("ls   ", ls_times))
            lines.append(timing_line("ft_ls", ft_times))
            lines.append(f"ratio: ft_ls / ls = {ft_median / ls_median:.3f}x (median)")
        else:
            lines.append("timing: skipped because at least one command returned non-zero")

        write_summary(args.results, lines)
        print("\n".join(lines))

        if not returncodes_same or not stdout_same or not stderr_same:
            print(
                f"diffs written to {args.results / 'stdout.diff'} and "
                f"{args.results / 'stderr.diff'}",
                file=sys.stderr,
            )
            return 1
        return 0
    except RuntimeError as exc:
        print(f"compare_ls.py: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
