from itertools import combinations
from pathlib import Path
from typing import Generator

from create_test_folders import create_test_folders

ALLOWED_FLAGS = ("R", "a", "l", "r", "t")


def gen_data(path: Path) -> Generator[list[str], None, None]:
    data = create_test_folders(path=path)

    for size in range(1, len(ALLOWED_FLAGS) + 1):
        for combo in combinations(ALLOWED_FLAGS, size):
            flags = "-" + "".join(combo)

            for entry in data.paths:
                yield [flags, str(entry)]

            for entry in data.files:
                yield [flags, str(entry)]

            for dir_entry in data.paths:
                for file_entry in data.files:
                    yield [flags, str(dir_entry), str(file_entry)]
