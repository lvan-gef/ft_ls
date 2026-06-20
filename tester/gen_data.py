from create_test_folders import Paths
from itertools import combinations
from typing import Generator


BASE_FLAGS = ("R", "a", "l", "r", "t")
KNOWN_BONUS_FLAGS = ("g", "u", "f", "d", "o", "Z")


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

    yield ["-R", "/mnt/bulk2/"]
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
