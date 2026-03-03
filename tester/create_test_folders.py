from pathlib import Path
import os
import time
import subprocess
import getpass
import shutil


def create_test_folders(path: Path) -> list[Path]:
    """Create all test fixture directories and return list of paths."""
    fullpaths = []

    # Simple directory (existing)
    fullpaths.append(create_simple(path))

    # Hidden files for -a testing
    fullpaths.append(create_hidden(path))

    # Timestamps for -t testing
    fullpaths.append(create_timestamps(path))

    # Recursive structure for -R testing
    fullpaths.append(create_recursive(path))

    # Symlinks for link handling
    fullpaths.append(create_symlinks(path))

    # Various permissions for -l testing
    fullpaths.append(create_permissions(path))

    # Various file sizes for -l testing
    fullpaths.append(create_sizes(path))

    # ACL/xattr marker tests for -l (Linux)
    fullpaths.append(create_xattrs(path))

    # Special character filenames
    fullpaths.append(create_special_chars(path))

    # Entries that require shell-style quoting in ls output
    fullpaths.append(create_quote_paths(path))
    fullpaths.append(create_quote_files(path))

    # Empty directory edge case
    fullpaths.append(create_empty(path))

    # Multiple path argument testing
    fullpaths.extend(create_multi_path(path))

    # Sorting edge cases
    fullpaths.append(create_sort_test(path))

    return fullpaths


def create_simple(path: Path) -> Path:
    """Create simple directory with basic files."""
    simple_path = path.joinpath("simple").absolute()
    simple_path.mkdir(parents=True, exist_ok=True)

    # Avoid spaces/quotes that trigger shell-escape quoting in ls
    files = [
        "abc.txt",
        "def.txt",
        "ghi.txt",
        "jkl.txt",
        "mno.txt",
        "pqr.txt",
        "stu.txt",
        "vwx.txt",
    ]

    for f in files:
        file_path = simple_path.joinpath(f)
        file_path.touch()

    return simple_path


def create_hidden(path: Path) -> Path:
    """Create directory with hidden files for -a testing."""
    hidden_path = path.joinpath("hidden").absolute()
    hidden_path.mkdir(parents=True, exist_ok=True)

    hidden_files = [".hidden1", ".hidden2", ".dotfile", ".config"]
    visible_files = ["visible1.txt", "visible2.txt", "visible3.txt"]

    for f in hidden_files + visible_files:
        file_path = hidden_path.joinpath(f)
        file_path.touch()

    return hidden_path


def create_timestamps(path: Path) -> Path:
    """Create directory with staggered mtimes for -t testing."""
    ts_path = path.joinpath("timestamps").absolute()
    ts_path.mkdir(parents=True, exist_ok=True)

    # Create files with 10-second intervals
    base_time = time.time()
    files = ["oldest.txt", "old.txt", "middle.txt", "recent.txt", "newest.txt"]

    for i, filename in enumerate(files):
        file_path = ts_path.joinpath(filename)
        file_path.touch()
        # Set modification time: oldest first, newest last
        mtime = base_time - (len(files) - 1 - i) * 10
        os.utime(file_path, (mtime, mtime))

    return ts_path


def create_recursive(path: Path) -> Path:
    """Create 3-level nested directory structure for -R and -Rt testing."""
    rec_path = path.joinpath("recursive").absolute()
    rec_path.mkdir(parents=True, exist_ok=True)

    # Root level file
    rec_path.joinpath("file_at_root.txt").touch()

    # level1_a with files and nested directories
    level1_a = rec_path.joinpath("level1_a")
    level1_a.mkdir(exist_ok=True)
    level1_a.joinpath("file_l1a.txt").touch()
    level1_a.joinpath(".hidden_l1a").touch()

    level2_a = level1_a.joinpath("level2_a")
    level2_a.mkdir(exist_ok=True)
    level2_a.joinpath("file_l2a.txt").touch()

    level3_a = level2_a.joinpath("level3_a")
    level3_a.mkdir(exist_ok=True)
    level3_a.joinpath("deepest.txt").touch()

    # level1_b with level2_b
    level1_b = rec_path.joinpath("level1_b")
    level1_b.mkdir(exist_ok=True)
    level2_b = level1_b.joinpath("level2_b")
    level2_b.mkdir(exist_ok=True)
    level2_b.joinpath("file_l2b.txt").touch()

    # level1_c (empty subdirectory)
    level1_c = rec_path.joinpath("level1_c")
    level1_c.mkdir(exist_ok=True)

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

    return rec_path


def create_symlinks(path: Path) -> Path:
    """Create directory with symlinks for link handling tests."""
    sym_path = path.joinpath("symlinks").absolute()
    sym_path.mkdir(parents=True, exist_ok=True)

    # Regular file
    regular_file = sym_path.joinpath("regular_file.txt")
    regular_file.write_text("content")

    # Symlink to file
    link_to_file = sym_path.joinpath("link_to_file")
    if not link_to_file.exists():
        link_to_file.symlink_to("regular_file.txt")

    # Symlink to directory (relative path to simple)
    link_to_dir = sym_path.joinpath("link_to_dir")
    if not link_to_dir.exists():
        link_to_dir.symlink_to("../simple")

    # Broken symlink
    broken_link = sym_path.joinpath("broken_link")
    if not broken_link.is_symlink():
        broken_link.symlink_to("nonexistent")

    return sym_path


def create_permissions(path: Path) -> Path:
    """Create directory with various permissions for -l testing."""
    perm_path = path.joinpath("permissions").absolute()
    perm_path.mkdir(parents=True, exist_ok=True)

    # Readable file (644)
    readable = perm_path.joinpath("readable.txt")
    readable.touch()
    readable.chmod(0o644)

    # Executable file (755)
    executable = perm_path.joinpath("executable.sh")
    executable.write_text("#!/bin/bash\necho hello\n")
    executable.chmod(0o755)

    # Read-only file (444)
    readonly = perm_path.joinpath("readonly.txt")
    readonly.touch()
    readonly.chmod(0o444)

    # Restricted directory for permission denied tests
    restricted = perm_path.joinpath("restricted")
    restricted.mkdir(exist_ok=True)
    restricted.joinpath("secret.txt").touch()
    # Note: We don't change permissions on restricted dir as it would
    # affect cleanup. Permission denied tests should be handled separately.

    return perm_path


def create_sizes(path: Path) -> Path:
    """Create directory with various file sizes for -l testing."""
    sizes_path = path.joinpath("sizes").absolute()
    sizes_path.mkdir(parents=True, exist_ok=True)

    # Empty file (0 bytes)
    sizes_path.joinpath("empty.txt").touch()

    # Tiny file (1 byte)
    sizes_path.joinpath("tiny.txt").write_text("x")

    # Small file (100 bytes)
    sizes_path.joinpath("small.txt").write_text("x" * 100)

    # Medium file (1KB)
    sizes_path.joinpath("medium.txt").write_text("x" * 1024)

    # Large file (10KB)
    sizes_path.joinpath("large.txt").write_text("x" * 10240)

    return sizes_path




def create_xattrs(path: Path) -> Path:
    """Create files with ACLs / xattrs to exercise ls -l marker behavior on Linux.

    GNU ls may show a trailing '+' in the mode string when a file has an ACL
    (stored in the 'system.posix_acl_access' xattr).
    """
    x_path = path.joinpath("xattrs").absolute()
    x_path.mkdir(parents=True, exist_ok=True)

    normal = x_path.joinpath("normal.txt")
    normal.write_text("normal\n")

    acl_file = x_path.joinpath("acl_file.txt")
    acl_file.write_text("acl\n")

    # Try to set an ACL so `ls -l` prints a '+' on the permissions field.
    # If setfacl is unavailable (or filesystem doesn't support ACLs), we skip.
    if shutil.which("setfacl") is not None:
        user = getpass.getuser()
        subprocess.run(
            ["setfacl", "-m", f"u:{user}:rw", str(acl_file)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

    # Optional: a harmless user.* xattr (GNU ls won't show it by default).
    if shutil.which("setfattr") is not None:
        subprocess.run(
            ["setfattr", "-n", "user.ft_ls_test", "-v", "hello", str(normal)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

    return x_path


def create_special_chars(path: Path) -> Path:
    """Create directory with special character filenames.

    Note: We avoid characters that trigger shell-escape quoting in ls
    (tabs, multiple spaces, mixed quotes) since matching that quoting
    logic is complex. Focus on simpler special chars.
    """
    special_path = path.joinpath("special_chars").absolute()
    special_path.mkdir(parents=True, exist_ok=True)

    # Dash at start (no quoting needed)
    special_path.joinpath("-dashstart.txt").touch()

    # Underscore variations (no quoting needed)
    special_path.joinpath("_underscore.txt").touch()
    special_path.joinpath("__double.txt").touch()

    # Numbers at start
    special_path.joinpath("123numbers.txt").touch()

    # Uppercase
    special_path.joinpath("UPPERCASE.txt").touch()

    # Mixed case
    special_path.joinpath("MixedCase.txt").touch()

    # Dots
    special_path.joinpath("file.name.ext").touch()

    # Long filename
    special_path.joinpath("a" * 50 + ".txt").touch()

    return special_path


def quote_case_names() -> list[str]:
    """Return filename cases that require quotes in ls output."""
    return [
        "space name",
        "single'quote",
        'double"quote',
        "space and 'single",
        'space and "double',
        "single'and\"double",
        'single\'and"double',
        'single"and\'double',
        "all 'and\" together",
    ]


def create_quote_paths(path: Path) -> Path:
    """Create directory entries whose path names require quoting."""
    quote_paths = path.joinpath("quote_paths").absolute()
    quote_paths.mkdir(parents=True, exist_ok=True)

    quote_paths.joinpath("plain").mkdir(exist_ok=True)

    for dirname in quote_case_names():
        case_dir = quote_paths.joinpath(dirname)
        case_dir.mkdir(exist_ok=True)
        case_dir.joinpath("inside.txt").touch()

    return quote_paths


def create_quote_files(path: Path) -> Path:
    """Create files whose names require quoting."""
    quote_files = path.joinpath("quote_files").absolute()
    quote_files.mkdir(parents=True, exist_ok=True)

    quote_files.joinpath("plain.txt").touch()

    for filename in quote_case_names():
        quote_files.joinpath(f"{filename}.txt").touch()

    return quote_files


def create_empty(path: Path) -> Path:
    """Create empty directory for edge case testing."""
    empty_path = path.joinpath("empty").absolute()
    empty_path.mkdir(parents=True, exist_ok=True)
    return empty_path


def create_multi_path(path: Path) -> list[Path]:
    """Create directories for multiple path argument testing."""
    multi_path = path.joinpath("multi_path").absolute()
    multi_path.mkdir(parents=True, exist_ok=True)

    # dir_a
    dir_a = multi_path.joinpath("dir_a")
    dir_a.mkdir(exist_ok=True)
    dir_a.joinpath("a1.txt").touch()
    dir_a.joinpath("a2.txt").touch()
    dir_a.joinpath("a3.txt").touch()

    # dir_b
    dir_b = multi_path.joinpath("dir_b")
    dir_b.mkdir(exist_ok=True)
    dir_b.joinpath("b1.txt").touch()
    dir_b.joinpath("b2.txt").touch()

    return [dir_a, dir_b]


def create_sort_test(path: Path) -> Path:
    """Create directory with sorting edge cases."""
    sort_path = path.joinpath("sort_test").absolute()
    sort_path.mkdir(parents=True, exist_ok=True)

    # Various filenames for sort testing
    files = [
        "AAA.txt",  # Uppercase
        "aaa.txt",  # Lowercase
        "111.txt",  # Numbers
        ".hidden_first",  # Hidden file
        "_underscore.txt",  # Underscore
        "ZZZ.txt",  # End of alphabet uppercase
        "zzz.txt",  # End of alphabet lowercase
        "MidCase.txt",  # Mixed case
    ]

    for f in files:
        sort_path.joinpath(f).touch()

    return sort_path
