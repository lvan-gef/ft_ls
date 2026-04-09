import getpass
import os
import shutil
import subprocess
import time
from pathlib import Path
from typing import Generator
from typing import NamedTuple


class Paths(NamedTuple):
    paths: list[Path]
    files: list[Path]
    cases: list[list[Path]]


def create_test_folders(path: Path) -> Paths:
    out_paths: list[Path] = []
    out_files: list[Path] = []

    # Simple directory (existing)
    new_path, files = create_simple(path=path)
    out_paths.append(new_path)
    out_files.extend(files)

    # Hidden files for -a testing
    new_path, files = create_hidden(path=path)
    out_paths.append(new_path)
    out_files.extend(files)

    # Timestamps for -t testing
    new_path, files = create_timestamps(path=path)
    out_paths.append(new_path)
    out_files.extend(files)

    # Recursive structure for -R testing
    new_path, files = create_recursive(path=path)
    out_paths.append(new_path)
    out_files.extend(files)

    # Symlinks for link handling
    new_path, files = create_symlinks(path=path)
    out_paths.extend(new_path)
    out_files.extend(files)

    # Various permissions for -l testing
    new_path, files = create_permissions(path=path)
    out_paths.extend(new_path)
    out_files.extend(files)

    # Various file sizes for -l testing
    new_path, files = create_sizes(path=path)
    out_paths.append(new_path)
    out_files.extend(files)

    # ACL/xattr marker tests for -l (Linux)
    new_path, files = create_xattrs(path=path)
    out_paths.append(new_path)
    out_files.extend(files)

    # Special character filenames
    new_path, files = create_special_chars(path=path)
    out_paths.append(new_path)
    out_files.extend(files)

    # Entries that require shell-style quoting in ls output
    new_path, files = create_quote_paths(path=path)
    out_paths.extend(new_path)
    out_files.extend(files)

    new_path, files = create_quote_files(path=path)
    out_paths.append(new_path)
    out_files.extend(files)

    # Empty directory edge case
    new_path, files = create_empty(path=path)
    out_paths.append(new_path)
    out_files.extend(files)

    # Multiple path argument testing
    new_path, files = create_multi_path(path=path)
    out_paths.extend(new_path)
    out_files.extend(files)

    # Sorting edge cases
    new_path, files = create_sort_test(path=path)
    out_paths.append(new_path)
    out_files.extend(files)

    return Paths(
        paths=out_paths,
        files=out_files,
        cases=create_curated_cases(path=path),
    )


def create_curated_cases(path: Path) -> list[list[Path]]:
    return [
        [path.joinpath('missing space')],
        [
            path.joinpath('quote_paths', 'plain'),
            path.joinpath('quote_paths', 'space name'),
        ],
        [
            path.joinpath('quote_files', 'plain.txt'),
            path.joinpath('quote_files', 'space name.txt'),
        ],
        [
            path.joinpath('quote_paths', 'plain'),
            path.joinpath('recursive', 'colon:dir'),
        ],
    ]


def create_simple(path: Path) -> tuple[Path, list[Path]]:
    simple_path = path.joinpath("simple").absolute()
    simple_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []

    files = ('abc.txt', 'def.txt', 'ghi.txt', 'jkl.txt',
             'mno.txt', 'pqr.txt', 'stu.txt', 'vwx.txt')

    for f in files:
        file_path = simple_path.joinpath(f)
        out_files.append(file_path)
        file_path.touch()

    return simple_path, out_files


def create_hidden(path: Path) -> tuple[Path, list[Path]]:
    hidden_path = path.joinpath('hidden').absolute()
    hidden_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []

    hidden_files = ['.hidden1', '.hidden2', '.dotfile', '.config']
    visible_files = ['visible1.txt', 'visible2.txt', 'visible3.txt']

    for f in hidden_files + visible_files:
        file_path = hidden_path.joinpath(f)
        out_files.append(file_path)
        file_path.touch()

    return hidden_path, out_files


def create_timestamps(path: Path) -> tuple[Path, list[Path]]:
    ts_path = path.joinpath('timestamps').absolute()
    ts_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []

    base_time = time.time()
    files = ['oldest.txt', 'old.txt', 'middle.txt', 'recent.txt', 'newest.txt']

    for i, filename in enumerate(files):
        file_path = ts_path.joinpath(filename)
        out_files.append(file_path)
        file_path.touch()
        # Set modification time: oldest first, newest last
        mtime = base_time - (len(files) - 1 - i) * 10
        os.utime(file_path, (mtime, mtime))

    return ts_path, out_files


def create_recursive(path: Path) -> tuple[Path, list[Path]]:
    rec_path = path.joinpath('recursive').absolute()
    rec_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []

    # Root level file
    file_path = rec_path.joinpath('file_at_root.txt')
    out_files.append(file_path)
    file_path.touch()

    level1_a = rec_path.joinpath('level1_a')
    level1_a.mkdir(exist_ok=True)
    file_path = level1_a.joinpath('file_l1a.txt')
    file_path.touch()
    out_files.append(file_path)

    file_path = level1_a.joinpath('.hidden_l1a')
    file_path.touch()
    out_files.append(file_path)

    level2_a = level1_a.joinpath('level2_a')
    level2_a.mkdir(exist_ok=True)
    file_path = level2_a.joinpath('file_l2a.txt')
    file_path.touch()
    out_files.append(file_path)

    level3_a = level2_a.joinpath('level3_a')
    level3_a.mkdir(exist_ok=True)
    file_path = level3_a.joinpath('deepest.txt')
    file_path.touch()
    out_files.append(file_path)

    # level1_b with level2_b
    level1_b = rec_path.joinpath('level1_b')
    level1_b.mkdir(exist_ok=True)
    level2_b = level1_b.joinpath('level2_b')
    level2_b.mkdir(exist_ok=True)
    file_path = level2_b.joinpath('file_l2b.txt')
    file_path.touch()
    out_files.append(file_path)

    # level1_c (empty subdirectory)
    level1_c = rec_path.joinpath('level1_c')
    level1_c.mkdir(exist_ok=True)

    quoted_dir = rec_path.joinpath('space dir')
    quoted_dir.mkdir(exist_ok=True)
    file_path = quoted_dir.joinpath('inside.txt')
    file_path.touch()
    out_files.append(file_path)

    colon_dir = rec_path.joinpath('colon:dir')
    colon_dir.mkdir(exist_ok=True)
    file_path = colon_dir.joinpath('inside.txt')
    file_path.touch()
    out_files.append(file_path)

    # Set different mtimes on directories for -Rt testing
    # level1_c = oldest, level1_b = middle, level1_a = newest
    base_time = time.time()
    dirs_with_times = [
        (level1_c, base_time - 30),  # oldest
        (level1_b, base_time - 20),  # middle
        (level1_a, base_time - 10),  # newest
    ]
    for dir_path, mtime in dirs_with_times:
        os.utime(dir_path, (mtime, mtime))

    return rec_path, out_files


def create_symlinks(path: Path) -> tuple[list[Path], list[Path]]:
    sym_path = path.joinpath('symlinks').absolute()
    sym_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []
    out_paths: list[Path] = []
    out_paths.append(sym_path)

    # Regular file
    regular_file = sym_path.joinpath('regular_file.txt')
    regular_file.write_text('content')
    out_files.append(regular_file)

    # Symlink to file
    link_to_file = sym_path.joinpath('link_to_file')
    if not link_to_file.exists():
        link_to_file.symlink_to('regular_file.txt')
        out_files.append(link_to_file)

    # Symlink to directory (relative path to simple)
    link_to_dir = sym_path.joinpath('link_to_dir')
    if not link_to_dir.exists():
        link_to_dir.symlink_to('../simple')
        out_paths.append(link_to_dir)

    # Broken symlink
    broken_link = sym_path.joinpath('broken_link')
    if not broken_link.is_symlink():
        broken_link.symlink_to('nonexistent')
        out_files.append(broken_link)

    return out_paths, out_files


def create_permissions(path: Path) -> tuple[list[Path], list[Path]]:
    perm_path = path.joinpath('permissions').absolute()
    perm_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []
    out_paths: list[Path] = []
    out_paths.append(perm_path)

    # Readable file (644)
    readable = perm_path.joinpath('readable.txt')
    readable.touch()
    readable.chmod(0o644)
    out_files.append(readable)

    # Executable file (755)
    executable = perm_path.joinpath('executable.sh')
    executable.write_text('#!/bin/bash\necho hello\n')
    executable.chmod(0o755)
    out_files.append(executable)

    # Read-only file (444)
    readonly = perm_path.joinpath('readonly.txt')
    readonly.touch()
    readonly.chmod(0o444)
    out_files.append(readonly)

    # Restricted directory for permission denied tests
    restricted = perm_path.joinpath('restricted')
    restricted.mkdir(exist_ok=True)
    restricted.joinpath('secret.txt').touch()
    restricted.chmod(0o000)
    out_paths.append(restricted)

    restricted_quoted = perm_path.joinpath('restricted dir')
    restricted_quoted.mkdir(exist_ok=True)
    restricted_quoted.joinpath('secret.txt').touch()
    restricted_quoted.chmod(0o000)
    out_paths.append(restricted_quoted)

    return out_paths, out_files


def create_sizes(path: Path) -> tuple[Path, list[Path]]:
    sizes_path = path.joinpath('sizes').absolute()
    sizes_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []

    # Empty file (0 bytes)
    file_path = sizes_path.joinpath('empty.txt')
    file_path.touch()
    out_files.append(file_path)

    # Tiny file (1 byte)
    file_path = sizes_path.joinpath('tiny.txt')
    file_path.write_text('x')
    out_files.append(file_path)

    # Small file (100 bytes)
    file_path = sizes_path.joinpath('small.txt')
    file_path.write_text('x' * 100)
    out_files.append(file_path)

    # Medium file (1KB)
    file_path = sizes_path.joinpath('medium.txt')
    file_path.write_text('x' * 1024)
    out_files.append(file_path)

    # Large file (10KB)
    file_path = sizes_path.joinpath('large.txt')
    file_path.write_text('x' * 10240)
    out_files.append(file_path)

    return sizes_path, out_files


def create_xattrs(path: Path) -> tuple[Path, list[Path]]:
    x_path = path.joinpath('xattrs').absolute()
    x_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []

    normal = x_path.joinpath('normal.txt')
    normal.write_text('normal\n')
    out_files.append(normal)

    acl_file = x_path.joinpath('acl_file.txt')
    acl_file.write_text('acl\n')

    # Try to set an ACL so `ls -l` prints a '+' on the permissions field.
    # If setfacl is unavailable (or filesystem doesn't support ACLs), we skip.
    if shutil.which('setfacl') is not None:
        user = getpass.getuser()
        subprocess.run(
            ['setfacl', '-m', f'u:{user}:rw', str(acl_file)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        out_files.append(acl_file)

    # Optional: a harmless user.* xattr (GNU ls won't show it by default).
    if shutil.which('setfattr') is not None:
        subprocess.run(
            ['setfattr', '-n', 'user.ft_ls_test', '-v', 'hello', str(normal)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

    return x_path, out_files


def create_special_chars(path: Path) -> tuple[Path, list[Path]]:
    special_path = path.joinpath('special_chars').absolute()
    special_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []
    names = [
        '-dashstart.txt',
        '_underscore.txt',
        '__double.txt',
        '123numbers.txt',
        'UPPERCASE.txt',
        'MixedCase.txt',
        'file.name.ext',
        'a' * 50 + '.txt',
    ]

    for name in names:
        file_path = special_path.joinpath(name)
        file_path.touch()
        out_files.append(file_path)

    for name in quote_case_names():
        file_path = special_path.joinpath(name)
        file_path.touch()
        out_files.append(file_path)

    for name in control_char_case_names():
        file_path = special_path.joinpath(name)
        file_path.touch()
        out_files.append(file_path)

    return special_path, out_files


def create_quote_paths(path: Path) -> tuple[list[Path], list[Path]]:
    quote_paths = path.joinpath('quote_paths').absolute()
    quote_paths.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []
    out_paths: list[Path] = []
    out_paths.append(quote_paths)

    plain_dir = quote_paths.joinpath('plain')
    plain_dir.mkdir(exist_ok=True)
    out_paths.append(plain_dir)

    for dirname in quote_case_names():
        case_dir = quote_paths.joinpath(dirname)
        case_dir.mkdir(exist_ok=True)
        out_paths.append(case_dir)
        file_path = case_dir.joinpath('inside.txt')
        file_path.touch()
        out_files.append(file_path)

    return out_paths, out_files


def create_quote_files(path: Path) -> tuple[Path, list[Path]]:
    quote_files = path.joinpath('quote_files').absolute()
    quote_files.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []

    file_path = quote_files.joinpath('plain.txt')
    file_path.touch()
    out_files.append(file_path)

    for filename in quote_case_names():
        file_path = quote_files.joinpath(f'{filename}.txt')
        file_path.touch()
        out_files.append(file_path)

    return quote_files, out_files


def create_empty(path: Path) -> tuple[Path, list[Path]]:
    empty_path = path.joinpath('empty').absolute()
    out_files: list[Path] = []
    empty_path.mkdir(parents=True, exist_ok=True)
    return empty_path, out_files


def create_multi_path(path: Path) -> tuple[list[Path], list[Path]]:
    multi_path = path.joinpath('multi_path').absolute()
    multi_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []
    out_paths: list[Path] = []

    # dir_a
    dir_a = multi_path.joinpath('dir_a')
    dir_a.mkdir(exist_ok=True)
    out_paths.append(dir_a)
    files = ['a1.txt', 'a2.txt', 'a3.txt']
    for file in files:
        path_file = dir_a.joinpath(file)
        path_file.touch()
        out_files.append(path_file)

    # dir_b
    dir_b = multi_path.joinpath('dir_b')
    dir_b.mkdir(exist_ok=True)
    out_paths.append(dir_b)
    files = ['b1.txt', 'b2.txt', 'b3.txt']
    for file in files:
        path_file = dir_b.joinpath(file)
        path_file.touch()
        out_files.append(path_file)

    return out_paths, out_files


def create_sort_test(path: Path) -> tuple[Path, list[Path]]:
    sort_path = path.joinpath("sort_test").absolute()
    sort_path.mkdir(parents=True, exist_ok=True)
    out_files: list[Path] = []

    files = [
        'AAA.txt',
        'aaa.txt',
        '111.txt',
        '.hidden_first',
        '_underscore.txt',
        'ZZZ.txt',
        'zzz.txt',
        'MidCase.txt',
    ]

    for f in files:
        path_file = sort_path.joinpath(f)
        path_file.touch()
        out_files.append(path_file)

    return sort_path, out_files


def quote_case_names() -> Generator[str, None, None]:
    for elem in (
        'space name',
        'single\'quote',
        'apostrophe\'s',
        'many\'apos\'trophes',
        'double"quote',
        'space and \'single',
        'space and "double',
        'single\'and"double',
        'single"and\'double',
        'all \'and" together',
        '[brackets]',
        '!bang',
        'equals=name',
        'utf-\u00e9',
        'cjk-\u6771',
        'smart-\u2019',
    ):
        yield elem


def control_char_case_names() -> Generator[str, None, None]:
    """Return cases with control characters that should be shell-escaped."""
    for elem in (
        'Icon\r',
        'carriage\rreturn',
    ):
        yield elem
