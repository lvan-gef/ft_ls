from create_test_folders import Paths
from itertools import combinations
from typing import Generator


ALLOWED_FLAGS = ("R", "a", "l", "r", "t")


def gen_data(paths: Paths) -> Generator[list[str], None, None]:
    yield ["-R", "/mnt/bulk2/"]

    for size in range(1, len(ALLOWED_FLAGS) + 1):
        for combo in combinations(ALLOWED_FLAGS, size):
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
