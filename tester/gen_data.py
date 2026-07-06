from create_test_folders import Paths
from itertools import combinations
from pathlib import Path
from typing import Generator


BASE_FLAGS = ("R", "a", "l", "r", "t")
KNOWN_BONUS_FLAGS = ("g", "u", "f", "d", "o")
NON_COMPARABLE_BONUS_FLAGS = ("G",)


def gen_data(
    paths: Paths,
    allowed_flags: tuple[str, ...] = BASE_FLAGS,
    bonus_flags: tuple[str, ...] = (),
) -> Generator[list[str], None, None]:
    yield ["-"]
    yield ["--", "-"]
    yield ["--"]
    yield ["--", ""]
    yield ["--", "." , "--"]

    yield ["-R", "/mnt/bulk2/code_projects/ft_ls/"]
    for size in range(1, len(allowed_flags) + 1):
        for combo in combinations(allowed_flags, size):
            flags = "-" + "".join(combo)

            # paths combies with -- and without
            for entry in paths.paths:
                yield [flags, str(entry)]

            for entry in paths.paths[:10]:
                yield ["--", flags, str(entry)]

            for entry in paths.paths[:10]:
                yield [flags, "--", str(entry)]

            # files combies with -- and without
            for entry in paths.files:
                yield [flags, str(entry)]

            for entry in paths.files[:10]:
                yield ["--", flags, str(entry)]

            for entry in paths.files[:10]:
                yield [flags, "--", str(entry)]

            # path and files combies with -- and without
            for dir_entry in paths.paths:
                for file_entry in paths.files:
                    yield [flags, str(dir_entry), str(file_entry)]

            for dir_entry in paths.paths[:10]:
                for file_entry in paths.files:
                    yield ["--", flags, str(dir_entry), str(file_entry)]

            for dir_entry in paths.paths[:10]:
                for file_entry in paths.files:
                    yield [flags, "--", str(dir_entry), str(file_entry)]

            # some edge cases with -- and without
            for case in paths.cases:
                yield [flags, *(str(entry) for entry in case)]

            for case in paths.cases[:10]:
                yield [flags, "--", *(str(entry) for entry in case)]

            for case in paths.cases[:10]:
                yield ["--", flags, *(str(entry) for entry in case)]

    if "d" in bonus_flags:
        for entry in paths.paths:
            yield ["-d", str(entry)]

    if "f" in bonus_flags:
        for entry in paths.paths:
            yield ["-f", str(entry)]


def gen_non_tty_data(
    paths: Paths, bonus_flags: tuple[str, ...] = ()
) -> Generator[list[str], None, None]:
    seen: set[tuple[str, ...]] = set()

    simple = _find_path(paths.paths, "simple")
    timestamps = _find_path(paths.paths, "timestamps")
    recursive = _find_path(paths.paths, "recursive")
    quote_paths = _find_path(paths.paths, "quote_paths")
    quote_files = _find_path(paths.paths, "quote_files")
    special_chars = _find_path(paths.paths, "special_chars")
    permissions = _find_path(paths.paths, "permissions")
    multi_dir = _find_suffix(paths.paths, "multi_path", "dir_a")
    quote_dir = _find_suffix(paths.paths, "quote_paths", "space name")
    quote_file = _find_suffix(paths.files, "quote_files", "space name.txt")
    broken_link = _find_suffix(paths.files, "symlinks", "broken_link")
    root = simple.parent if simple is not None else None

    base_cases = [
        _case((), simple),
        _case(("-R",), recursive),
        _case(("-a",), special_chars),
        _case(("-l",), permissions),
        _case(("-l",), quote_files),
        _case(("-R",), quote_paths),
        _case(("-t",), timestamps),
        _case(("-rt",), timestamps),
        _case(("-lR",), recursive),
        _case(("-la",), special_chars),
        _case(("-lt",), timestamps),
        _case(("-lr",), permissions),
        _case((), multi_dir, quote_file),
        _case((), root / "missing space" if root is not None else None),
        _case((), broken_link),
    ]

    for case in base_cases:
        yield from _yield_unique(case, seen)

    for flag in bonus_flags:
        target = _bonus_target(
            flag, simple, timestamps, quote_files, special_chars, quote_dir
        )
        flag_cases = [
            _case((f"-{flag}",), target),
            _case((f"-l{flag}",), target),
            _case((f"-R{flag}",), recursive),
        ]

        if flag == "u":
            flag_cases.append(_case(("-ltu",), timestamps))
        elif flag == "d":
            flag_cases.append(_case(("-ld",), quote_dir))
        elif flag == "f":
            flag_cases.append(_case(("-af",), special_chars))

        for case in flag_cases:
            yield from _yield_unique(case, seen)

    for first, second in combinations(bonus_flags, 2):
        flags = first + second
        for case in (
            _case((f"-l{flags}",), quote_files),
            _case((f"-lR{flags}",), recursive),
        ):
            yield from _yield_unique(case, seen)

    if bonus_flags:
        flags = "".join(bonus_flags)
        for case in (
            _case((f"-l{flags}",), quote_files),
            _case((f"-lR{flags}",), recursive),
        ):
            yield from _yield_unique(case, seen)


def _find_path(entries: list[Path], name: str) -> Path | None:
    for entry in entries:
        if entry.name == name:
            return entry

    return None


def _find_suffix(entries: list[Path], *suffix: str) -> Path | None:
    for entry in entries:
        entry_suffix = entry.parts[-len(suffix) :]
        if len(entry.parts) >= len(suffix) and entry_suffix == suffix:
            return entry

    return None


def _case(flags: tuple[str, ...], *entries: Path | None) -> list[str] | None:
    case = [*flags]
    for entry in entries:
        if entry is None:
            return None
        case.append(str(entry))

    return case


def _yield_unique(
    case: list[str] | None, seen: set[tuple[str, ...]]
) -> Generator[list[str], None, None]:
    if case is None:
        return

    key = tuple(case)
    if key in seen:
        return

    seen.add(key)
    yield case


def _bonus_target(
    flag: str,
    simple: Path | None,
    timestamps: Path | None,
    quote_files: Path | None,
    special_chars: Path | None,
    quote_dir: Path | None,
) -> Path | None:
    if flag == "u":
        return timestamps
    if flag == "d":
        return quote_dir
    if flag == "f":
        return special_chars
    if flag in ("g", "o"):
        return quote_files

    return simple
