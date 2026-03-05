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
from collections import namedtuple
from itertools import permutations
from pathlib import Path
from typing import Generator


from create_test_folders import create_test_folders

CommandResult = namedtuple('CommandResult', ['stdout', 'stderr', 'returncode'])
ALLOWED_FLAGS = ["R", "a", "l", "r", "t"]
DEBUG = True
TERMINAL_MIN = 80  # include
TERMINAL_MAX = 513  # exclude
ft_ls = "./ft_ls"
if DEBUG:
    own_bin = f"{ft_ls}_d"


def main() -> None:
    test_path = Path.cwd().joinpath("ft_ls_tester")
    test_paths = create_test_folders(path=test_path)

    if test_path.exists():
        shutil.rmtree(test_path)

    # Phase 1: Invalid flags (no terminal width dependency)
    subprocess.run("make fclean", shell=True, capture_output=True)
    # compile_ls()
    # print("=" * 60)
    # print("Phase 1: Invalid Flags")
    # print("=" * 60)
    # invalid_flags()

    # Phase 2-5: Flag combination and feature tests
    print("=" * 60)
    print("Phase 2-5: Flag Combinations and Feature Tests")
    print("=" * 60)
    flag_combination_tests(test_paths=test_paths)
    subprocess.run("make fclean", shell=True, capture_output=True)

    # Phase 6: Terminal width tests
    # for term_size in range(TERMINAL_MIN, TERMINAL_MAX):
    #     compile_ls(term_size=term_size)
    #     print("-" * 10, "Test column width:", term_size, "-" * 10)
    #     simple_tests(term_size=term_size, test_files=test_files)
    #     subprocess.run("make fclean", shell=True, capture_output=True)

    shutil.rmtree(test_path)
    print("=" * 60)
    print("All tests passed!")
    print("=" * 60)


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

        assert result.returncode == 1, f"Flag -{lttr} should return exit code 1"
        assert result.stdout == "", f"Flag -{lttr} should have empty stdout"
        assert result.stderr != "", f"Flag -{lttr} should have stderr message"
        counter += 1

    print(f"  Tested {counter} invalid flags: all passed")


def flag_combination_tests(test_paths: list[Path]) -> None:
    """Phase 2-5: Test all flag combinations against test fixtures."""

    # Phase 2: Single flag tests
    print("\n  Phase 2: Single Flag Tests")
    flags_counter = 0
    dirs_counter = 0
    for flag in generate_flag():
        for test_dir in test_paths:
            ft_cmd = [ft_ls, flag, str(test_dir)] if flag else [ft_ls, str(test_dir)]
            ls_cmd = ['ls', flag, str(test_dir)] if flag else ['ls', str(test_dir)]

            assert_command(ft_cmd=ft_cmd,
                           ls_cmd=ls_cmd,
                           msg='single flag test failed')
            dirs_counter += 1
        flags_counter += 1
    print(f"    Tested {flags_counter} flag variants x {dirs_counter} dirs")

    # Phase 3: Flag combinations (2-5 flags)
    print("\n  Phase 3: Flag Combination Tests")
    flags_counter = 0
    dirs_counter = 0
    for flags in generate_flag_combinations():
        for test_dir in test_paths:
            ft_cmd = [ft_ls, flags, str(test_dir)]
            ls_cmd = ['ls', flags, str(test_dir)]

            assert_command(ft_cmd=ft_cmd,
                           ls_cmd=ls_cmd,
                           msg='single flag test failed')
            dirs_counter += 1
        flags_counter += 1
    print(f"    Tested {flags_counter} combinations x {dirs_counter} dirs")

    # Phase 4: Feature-specific tests
    print("\n  Phase 4: Feature-Specific Tests")
    feature_specific_tests(test_paths=test_paths)

    # Phase 5: Edge case tests
    print("\n  Phase 5: Edge Case Tests")
    edge_case_tests(test_path)


# def assert_contains(output: str, needle: str, cmd: list[str], msg: str) -> None:
#     """Assert output contains needle, showing command on failure."""
#     if needle not in output:
#         print(f"\nCommand: {' '.join(cmd)}", file=sys.stderr)
#         print(f"Output:", file=sys.stderr)
#         print(output, file=sys.stderr)
#         print(f"Expected to contain: {needle}", file=sys.stderr)
#         raise AssertionError(f"{msg}: {' '.join(cmd)}")
#
#
# def assert_not_contains(output: str, needle: str, cmd: list[str], msg: str) -> None:
#     """Assert output does not contain needle, showing command on failure."""
#     if needle in output:
#         print(f"\nCommand: {' '.join(cmd)}", file=sys.stderr)
#         print(f"Output:", file=sys.stderr)
#         print(output, file=sys.stderr)
#         print(f"Expected NOT to contain: {needle}", file=sys.stderr)
#         raise AssertionError(f"{msg}: {' '.join(cmd)}")
#

# def feature_specific_tests(test_paths: list[Path]) -> None:
#     """Phase 4: Test specific features in detail."""
#
#     # 4.1 Hidden files (-a) test
#     hidden_dir = test_paths / "hidden"
#     # Without -a: should not show hidden files
#     cmd_no_a = [ft_ls, str(hidden_dir)]
#     output_no_a = run_with_pty(cmd=cmd_no_a)
#     assert_not_contains(
#         output_no_a, ".hidden1", cmd_no_a, "Hidden files should not appear without -a"
#     )
#     assert_contains(
#         output_no_a, "visible1.txt", cmd_no_a, "Visible files should appear without -a"
#     )
#
#     # With -a: should show hidden files
#     cmd_with_a = [ft_ls, "-a", str(hidden_dir)]
#     output_with_a = run_with_pty(cmd=cmd_with_a)
#     assert_contains(
#         output_with_a, ".hidden1", cmd_with_a, "Hidden files should appear with -a"
#     )
#     assert_contains(output_with_a, ".", cmd_with_a, ". should appear with -a")
#     assert_contains(output_with_a, "..", cmd_with_a, ".. should appear with -a")
#     print("    4.1 Hidden files (-a): passed")
#
#     # 4.2 Time sorting (-t) test
#     ts_dir = test_paths / "timestamps"
#     assert_command(
#         [ft_ls, "-t", str(ts_dir)],
#         ["ls", "-t", str(ts_dir)],
#         "Time sorting should match ls -t",
#     )
#     print("    4.2 Time sorting (-t): passed")
#
#     # 4.3 Reverse sorting (-r) test
#     simple_dir = test_paths / "simple"
#     assert_command(
#         [ft_ls, "-r", str(simple_dir)],
#         ["ls", "-r", str(simple_dir)],
#         "Reverse sorting should match ls -r",
#     )
#     print("    4.3 Reverse sorting (-r): passed")
#
#     # 4.4 Recursive listing (-R) test
#     rec_dir = test_paths / "recursive"
#     cmd_R = [ft_ls, "-R", str(rec_dir)]
#     output_R = assert_command(
#         cmd_R, ["ls", "-R", str(rec_dir)], "Recursive listing should match ls -R"
#     )
#     # Verify all levels are traversed
#     assert_contains(output_R, "level1_a", cmd_R, "level1_a should appear in -R output")
#     assert_contains(output_R, "level2_a", cmd_R, "level2_a should appear in -R output")
#     if "level3_a" not in output_R and "deepest.txt" not in output_R:
#         print(f"\nCommand: {' '.join(cmd_R)}", file=sys.stderr)
#         print(f"Output:\n{output_R}", file=sys.stderr)
#         raise AssertionError(
#             f"Deepest level should appear in -R output: {' '.join(cmd_R)}"
#         )
#     print("    4.4 Recursive listing (-R): passed")
#
#     # 4.5 Long format (-l) test
#     perm_dir = test_paths / "permissions"
#     cmd_l = [ft_ls, "-l", str(perm_dir)]
#     output_l = assert_command(
#         cmd_l, ["ls", "-l", str(perm_dir)], "Long format should match ls -l"
#     )
#     # Verify permission columns appear
#     assert_contains(
#         output_l, "rw", cmd_l, "Permission string should appear in -l output"
#     )
#     print("    4.5 Long format (-l): passed")
#     # 4.5b ACL marker (+) test (Linux)
#     xattrs_dir = test_paths / "xattrs"
#     cmd_acl = [ft_ls, "-l", str(xattrs_dir)]
#     ft_out = run_with_pty(cmd=cmd_acl)
#     ls_out = run_with_pty(cmd=["ls", "-l", str(xattrs_dir)])
#
#     # If the fixture successfully set an ACL, GNU ls should show a '+' on acl_file.txt.
#     # If not (no setfacl/ACL support), we skip the marker assertion but still require output match.
#     if ft_out != ls_out:
#         assert_command(
#             cmd_acl,
#             ["ls", "-l", str(xattrs_dir)],
#             "ACL dir long format should match ls -l",
#         )
#
#     # Only assert the marker when ls actually prints it (depends on setfacl + fs support).
#     for line in ls_out.splitlines():
#         if line.rstrip().endswith("acl_file.txt"):
#             if "+" in line.split()[0]:
#                 for fline in ft_out.splitlines():
#                     if fline.rstrip().endswith("acl_file.txt"):
#                         if "+" not in fline.split()[0]:
#                             print(f"Command: {' '.join(cmd_acl)}", file=sys.stderr)
#                             print("ft_ls line:", file=sys.stderr)
#                             print(line, file=sys.stderr)
#                             print("ls line:", file=sys.stderr)
#                             print(line, file=sys.stderr)
#                             raise AssertionError("ACL marker '+' missing from ft_ls -l output")
#                 break
#
#     print("    4.5b ACL marker (+) if supported: passed")
#
#     # 4.6 Recursive time sort (-Rt) test
#
#
#     rec_dir = test_paths / "recursive"
#     assert_command(
#         [ft_ls, "-Rt", str(rec_dir)],
#         ["ls", "-Rt", str(rec_dir)],
#         "Recursive time sort should match ls -Rt",
#     )
#     print("    4.6 Recursive time sort (-Rt): passed")


# def edge_case_tests(test_path: Path) -> None:
#     """Phase 5: Test edge cases."""
#     all_flags = get_all_flags()
#     quote_cases = [
#         "space name",
#         "single'quote",
#         'double"quote',
#         "space and 'single",
#         'space and "double',
#         "single'and\"double",
#         "all 'and\" together",
#     ]
#
#     # 5.1 Symlinks directory test (all flag combinations)
#     sym_dir = test_path / "symlinks"
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(sym_dir)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(sym_dir)]
#         assert_command(
#             ft_cmd, ls_cmd, f"Symlink dir with {flags or 'no flags'} should match"
#         )
#     print(f"    5.1 Symlinks directory ({len(all_flags)} flag combos): passed")
#
#     # 5.2 Symlink as argument tests (all flag combinations)
#     link_to_file = test_path / "symlinks" / "link_to_file"
#     link_to_dir = test_path / "symlinks" / "link_to_dir"
#     broken_link = test_path / "symlinks" / "broken_link"
#     regular_file = test_path / "simple" / "abc.txt"
#     regular_dir = test_path / "simple"
#
#     # 5.2a Symlink to file as argument
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(link_to_file)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(link_to_file)]
#         assert_command(
#             ft_cmd, ls_cmd, f"Symlink to file with {flags or 'no flags'} should match"
#         )
#     print(f"    5.2a Symlink to file ({len(all_flags)} flag combos): passed")
#
#     # 5.2b Symlink to directory as argument
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(link_to_dir)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(link_to_dir)]
#         assert_command(
#             ft_cmd, ls_cmd, f"Symlink to dir with {flags or 'no flags'} should match"
#         )
#     print(f"    5.2b Symlink to directory ({len(all_flags)} flag combos): passed")
#
#     # 5.2c Broken symlink as argument
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(broken_link)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(broken_link)]
#         result_ft = subprocess.run(ft_cmd, capture_output=True, text=True)
#         result_ls = subprocess.run(ls_cmd, capture_output=True, text=True)
#         if result_ft.returncode != result_ls.returncode:
#             print(f"\nCommand: {' '.join(ft_cmd)}", file=sys.stderr)
#             print(f"ft_ls returncode: {result_ft.returncode}", file=sys.stderr)
#             print(f"ls returncode: {result_ls.returncode}", file=sys.stderr)
#             raise AssertionError(
#                 f"Broken symlink return code should match ls: {' '.join(ft_cmd)}"
#             )
#     print(f"    5.2c Broken symlink ({len(all_flags)} flag combos): passed")
#
#     # 5.2d Symlink to file + regular file
#     for flags in all_flags:
#         ft_cmd = (
#             [ft_ls]
#             + ([flags] if flags else [])
#             + [str(link_to_file), str(regular_file)]
#         )
#         ls_cmd = (
#             ["ls"] + ([flags] if flags else []) + [str(link_to_file), str(regular_file)]
#         )
#         assert_command(
#             ft_cmd, ls_cmd, f"Symlink+file with {flags or 'no flags'} should match"
#         )
#     print(
#         f"    5.2d Symlink to file + regular file ({len(all_flags)} flag combos): passed"
#     )
#
#     # 5.2e Symlink to dir + regular dir
#     for flags in all_flags:
#         ft_cmd = (
#             [ft_ls]
#             + ([flags] if flags else [])
#             + [str(link_to_dir), str(regular_dir)]
#         )
#         ls_cmd = (
#             ["ls"] + ([flags] if flags else []) + [str(link_to_dir), str(regular_dir)]
#         )
#         assert_command(
#             ft_cmd, ls_cmd, f"Symlink+dir with {flags or 'no flags'} should match"
#         )
#     print(
#         f"    5.2e Symlink to dir + regular dir ({len(all_flags)} flag combos): passed"
#     )
#
#     # 5.2f Mixed: symlink to file, symlink to dir, regular file, regular dir
#     for flags in all_flags:
#         ft_cmd = (
#             [ft_ls]
#             + ([flags] if flags else [])
#             + [str(link_to_file), str(link_to_dir), str(regular_file), str(regular_dir)]
#         )
#         ls_cmd = (
#             ["ls"]
#             + ([flags] if flags else [])
#             + [str(link_to_file), str(link_to_dir), str(regular_file), str(regular_dir)]
#         )
#         assert_command(
#             ft_cmd,
#             ls_cmd,
#             f"Mixed symlinks+regular with {flags or 'no flags'} should match",
#         )
#     print(f"    5.2f Mixed symlinks + regular ({len(all_flags)} flag combos): passed")
#
#     # 5.2g Broken symlink + regular file (partial failure)
#     for flags in all_flags:
#         ft_cmd = (
#             [ft_ls]
#             + ([flags] if flags else [])
#             + [str(broken_link), str(regular_file)]
#         )
#         ls_cmd = (
#             ["ls"] + ([flags] if flags else []) + [str(broken_link), str(regular_file)]
#         )
#         result_ft = subprocess.run(ft_cmd, capture_output=True, text=True)
#         result_ls = subprocess.run(ls_cmd, capture_output=True, text=True)
#         if result_ft.returncode != result_ls.returncode:
#             print(f"\nCommand: {' '.join(ft_cmd)}", file=sys.stderr)
#             print(f"ft_ls returncode: {result_ft.returncode}", file=sys.stderr)
#             print(f"ls returncode: {result_ls.returncode}", file=sys.stderr)
#             raise AssertionError(
#                 f"Broken symlink+file return code should match ls: {' '.join(ft_cmd)}"
#             )
#         if result_ft.stderr == "":
#             raise AssertionError(
#                 f"Broken symlink+file should produce stderr: {' '.join(ft_cmd)}"
#             )
#         if result_ft.stdout == "":
#             raise AssertionError(
#                 f"Broken symlink+file should produce stdout for regular file: {' '.join(ft_cmd)}"
#             )
#     print(
#         f"    5.2g Broken symlink + regular file ({len(all_flags)} flag combos): passed"
#     )
#
#     # 5.3 Special characters test (all flag combinations)
#     special_dir = test_path / "special_chars"
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(special_dir)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(special_dir)]
#         assert_command(
#             ft_cmd, ls_cmd, f"Special chars with {flags or 'no flags'} should match"
#         )
#     print(f"    5.3 Special characters ({len(all_flags)} flag combos): passed")
#
#     # 5.4 Empty directory test (all flag combinations)
#     empty_dir = test_path / "empty"
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(empty_dir)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(empty_dir)]
#         assert_command(
#             ft_cmd, ls_cmd, f"Empty dir with {flags or 'no flags'} should match"
#         )
#     print(f"    5.4 Empty directory ({len(all_flags)} flag combos): passed")
#
#     # 5.5 Multiple directories test (all flag combinations)
#     dir_a = test_path / "multi_path" / "dir_a"
#     dir_b = test_path / "multi_path" / "dir_b"
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(dir_a), str(dir_b)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(dir_a), str(dir_b)]
#         assert_command(
#             ft_cmd, ls_cmd, f"Multiple dirs with {flags or 'no flags'} should match"
#         )
#     print(f"    5.5 Multiple directories ({len(all_flags)} flag combos): passed")
#
#     # 5.6 Non-existent path test
#     cmd_nonexist = [ft_ls, "/nonexistent/path"]
#     result_ft = subprocess.run(cmd_nonexist, capture_output=True, text=True)
#     result_ls = subprocess.run(
#         ["ls", "/nonexistent/path"], capture_output=True, text=True
#     )
#     if result_ft.returncode != result_ls.returncode:
#         print(f"\nCommand: {' '.join(cmd_nonexist)}", file=sys.stderr)
#         print(f"ft_ls returncode: {result_ft.returncode}", file=sys.stderr)
#         print(f"ls returncode: {result_ls.returncode}", file=sys.stderr)
#         raise AssertionError(
#             f"Non-existent path return code should match ls: {' '.join(cmd_nonexist)}"
#         )
#     if result_ft.stderr == "":
#         print(f"\nCommand: {' '.join(cmd_nonexist)}", file=sys.stderr)
#         print(f"stderr was empty", file=sys.stderr)
#         raise AssertionError(
#             f"Non-existent path should produce stderr: {' '.join(cmd_nonexist)}"
#         )
#     print("    5.6 Non-existent path: passed")
#
#     # 5.7 Mixed existent and non-existent paths test
#     cmd_mixed = [ft_ls, "/nonexistent/path", str(regular_dir)]
#     result_ft = subprocess.run(cmd_mixed, capture_output=True, text=True)
#     result_ls = subprocess.run(
#         ["ls", "/nonexistent/path", str(regular_dir)], capture_output=True, text=True
#     )
#     if result_ft.returncode != result_ls.returncode:
#         print(f"\nCommand: {' '.join(cmd_mixed)}", file=sys.stderr)
#         print(f"ft_ls returncode: {result_ft.returncode}", file=sys.stderr)
#         print(f"ls returncode: {result_ls.returncode}", file=sys.stderr)
#         raise AssertionError(
#             f"Mixed paths return code should match ls: {' '.join(cmd_mixed)}"
#         )
#     if result_ft.stderr == "":
#         print(f"\nCommand: {' '.join(cmd_mixed)}", file=sys.stderr)
#         print(f"stderr was empty", file=sys.stderr)
#         raise AssertionError(
#             f"Mixed paths should produce stderr for non-existent: {' '.join(cmd_mixed)}"
#         )
#     if result_ft.stdout == "":
#         print(f"\nCommand: {' '.join(cmd_mixed)}", file=sys.stderr)
#         print(f"stdout was empty", file=sys.stderr)
#         raise AssertionError(
#             f"Mixed paths should produce stdout for existent: {' '.join(cmd_mixed)}"
#         )
#     print("    5.7 Mixed existent/non-existent paths: passed")
#
#     # 5.8 Single file argument test (all flag combinations)
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(regular_file)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(regular_file)]
#         assert_command(
#             ft_cmd, ls_cmd, f"Single file with {flags or 'no flags'} should match"
#         )
#     print(f"    5.8 Single file argument ({len(all_flags)} flag combos): passed")
#
#     # 5.9 Multiple file arguments test (all flag combinations)
#     file1 = test_path / "simple" / "abc.txt"
#     file2 = test_path / "simple" / "def.txt"
#     file3 = test_path / "simple" / "ghi.txt"
#     for flags in all_flags:
#         ft_cmd = (
#             [ft_ls]
#             + ([flags] if flags else [])
#             + [str(file1), str(file2), str(file3)]
#         )
#         ls_cmd = (
#             ["ls"] + ([flags] if flags else []) + [str(file1), str(file2), str(file3)]
#         )
#         assert_command(
#             ft_cmd, ls_cmd, f"Multiple files with {flags or 'no flags'} should match"
#         )
#     print(f"    5.9 Multiple file arguments ({len(all_flags)} flag combos): passed")
#
#     # 5.10 Sort test directory (all flag combinations)
#     sort_dir = test_path / "sort_test"
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(sort_dir)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(sort_dir)]
#         assert_command(
#             ft_cmd, ls_cmd, f"Sort test with {flags or 'no flags'} should match"
#         )
#     print(f"    5.10 Sort edge cases ({len(all_flags)} flag combos): passed")
#
#     # 5.11 Sizes directory (all flag combinations)
#     sizes_dir = test_path / "sizes"
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(sizes_dir)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(sizes_dir)]
#         assert_command(
#             ft_cmd, ls_cmd, f"Sizes with {flags or 'no flags'} should match"
#         )
#     print(f"    5.11 Sizes directory ({len(all_flags)} flag combos): passed")
#
#     # 5.12 Quote-required path entries test (all flag combinations)
#     quote_paths_dir = test_path / "quote_paths"
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(quote_paths_dir)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(quote_paths_dir)]
#         assert_command(
#             ft_cmd,
#             ls_cmd,
#             f"Quote-required paths with {flags or 'no flags'} should match",
#         )
#     print(f"    5.12 Quote-required paths ({len(all_flags)} flag combos): passed")
#
#     # 5.13 Quote-required file entries test (all flag combinations)
#     quote_files_dir = test_path / "quote_files"
#     for flags in all_flags:
#         ft_cmd = [ft_ls] + ([flags] if flags else []) + [str(quote_files_dir)]
#         ls_cmd = ["ls"] + ([flags] if flags else []) + [str(quote_files_dir)]
#         assert_command(
#             ft_cmd,
#             ls_cmd,
#             f"Quote-required files with {flags or 'no flags'} should match",
#         )
#     print(f"    5.13 Quote-required files ({len(all_flags)} flag combos): passed")
#
#     # 5.14 Quote-required paths as arguments (all flag combinations)
#     quote_path_args = [test_path / "quote_paths" / name for name in quote_cases]
#     for flags in all_flags:
#         ft_cmd = (
#             [ft_ls]
#             + ([flags] if flags else [])
#             + [str(path_arg) for path_arg in quote_path_args]
#         )
#         ls_cmd = (
#             ["ls"]
#             + ([flags] if flags else [])
#             + [str(path_arg) for path_arg in quote_path_args]
#         )
#         assert_command(
#             ft_cmd,
#             ls_cmd,
#             f"Quote-required path args with {flags or 'no flags'} should match",
#         )
#     print(f"    5.14 Quote path arguments ({len(all_flags)} flag combos): passed")
#
#     # 5.15 Quote-required files as arguments (all flag combinations)
#     quote_file_args = [
#         test_path / "quote_files" / f"{filename}.txt" for filename in quote_cases
#     ]
#     for flags in all_flags:
#         ft_cmd = (
#             [ft_ls]
#             + ([flags] if flags else [])
#             + [str(file_arg) for file_arg in quote_file_args]
#         )
#         ls_cmd = (
#             ["ls"]
#             + ([flags] if flags else [])
#             + [str(file_arg) for file_arg in quote_file_args]
#         )
#         assert_command(
#             ft_cmd,
#             ls_cmd,
#             f"Quote-required file args with {flags or 'no flags'} should match",
#         )
#     print(f"    5.15 Quote file arguments ({len(all_flags)} flag combos): passed")
#
#     # 5.16 Recursive output comparison know paths
#     paths = [Path("/mnt/bulk"), Path().home()]
#
#     for i, p in enumerate(paths):
#         if not p.exists():
#             print(f"    5.16{string.ascii_lowercase[i]} Recursive {p} (-R): skipped (path not found)")
#         else:
#             ft_cmd = [ft_ls, "-R", str(p)]
#             ls_cmd = ["ls", "-R", str(p)]
#             assert_command(ft_cmd, ls_cmd, f"Recursive {p} with -R should match")
#             print("    5.16 Recursive /mnt/bulk (-R): passed")
#
#
# def compare_output(flags: str, path: str, cols: int = 80) -> None:
#     """Compare ft_ls output with system ls output."""
#     if flags:
#         ft_cmd = [ft_ls, flags, path]
#         ls_cmd = ["ls", flags, path]
#     else:
#         ft_cmd = [ft_ls, path]
#         ls_cmd = ["ls", path]
#
#     ft_output = run_with_pty(cmd=ft_cmd, cols=cols)
#     ls_output = run_with_pty(cmd=ls_cmd, cols=cols)
#
#     if ft_output != ls_output:
#         print(f"\nMismatch for: {' '.join(ft_cmd)}", file=sys.stderr)
#         print(f"ls output:\n{ls_output}", file=sys.stderr)
#         print("-" * 40, file=sys.stderr)
#         print(f"ft_ls output:\n{ft_output}", file=sys.stderr)
#         print("-" * 40, file=sys.stderr)
#
#         # Show diff
#         seqm = difflib.SequenceMatcher(None, ft_output, ls_output)
#         for opcode, a0, a1, b0, b1 in seqm.get_opcodes():
#             if opcode == "replace":
#                 print(
#                     f"Replace:\n'{ft_output[a0:a1]}'\nWith:\n'{ls_output[b0:b1]}'",
#                     file=sys.stderr,
#                 )
#             elif opcode == "insert":
#                 print(f"Insert:\n'{ls_output[b0:b1]}'", file=sys.stderr)
#             elif opcode == "delete":
#                 print(f"Delete:\n'{ft_output[a0:a1]}'", file=sys.stderr)
#
#         raise AssertionError(f"Output mismatch for: {' '.join(ft_cmd)}")


def generate_flag_combinations() -> Generator[str, None, None]:
    """Generate all permutations of flags."""

    for index in range(1, len(ALLOWED_FLAGS) + 1):
        for perm in permutations(ALLOWED_FLAGS[:index]):
            yield f'-{"".join(perm)}'


def generate_flag() -> Generator[str, None, None]:
    """Generate all single flags."""

    yield ""
    for flag in ALLOWED_FLAGS:
        yield f'-{"".join(flag)}'


# def get_all_flags() -> list[str]:
#     """Get all flag variants: no flags, single flags, and combinations."""
#     all_flags = [""]  # no flags
#     for flag in ALLOWED_FLAGS:
#         all_flags.append(f"-{flag}")
#     all_flags.extend(generate_flag_combinations())
#     return all_flags


def assert_command(ft_cmd: list[str], ls_cmd: list[str], msg: str) -> None:
    """Assert ft_ls output matches ls output, showing diff on failure."""
    ft: CommandResult = run_with_pty(cmd=ft_cmd)
    ls: CommandResult = run_with_pty(cmd=ls_cmd)

    if ft.stdout != ls.stdout:
        print(f"\nMismatch on stdout for: {' '.join(ft_cmd)}", file=sys.stderr)
        print(f"ls stdout: {ls.stdout}", file=sys.stderr)
        print("-" * 40, file=sys.stderr)
        print(f"ft_ls stdout: {ft.stdout}", file=sys.stderr)

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

    if ft.stderr != ls.stderr:
        print(f"\nMismatch on stderr for: {' '.join(ft_cmd)}", file=sys.stderr)
        print(f"ls stderr: {ls.stderr}", file=sys.stderr)
        print("-" * 40, file=sys.stderr)
        print(f"ft_ls stderr: {ft.stderr}", file=sys.stderr)

        seqm = difflib.SequenceMatcher(None, ft.stderr, ls.stderr)
        for opcode, a0, a1, b0, b1 in seqm.get_opcodes():
            if opcode == "replace":
                print(
                    f"Replace:\n'{ft.stderr[a0:a1]}'\nWith:\n'{ls.stderr[b0:b1]}'",
                    file=sys.stderr,
                )
            elif opcode == "insert":
                print(f"Insert:\n'{ls.stderr[b0:b1]}'", file=sys.stderr)
            elif opcode == "delete":
                print(f"Delete:\n'{ft.stderr[a0:a1]}'", file=sys.stderr)

        raise AssertionError(f"{msg}: {' '.join(ft_cmd)}")

    if ft.returncode != ls.returncode:
        print(f"\nMismatch on returncode for: {' '.join(ft_cmd)}", file=sys.stderr)
        print(f"ls returncode {ls.returncode}:", file=sys.stderr)
        print("-" * 40, file=sys.stderr)
        print(f"ft_ls returncode: {ft.returncode}", file=sys.stderr)


def run_with_pty(cmd: list[str], cols: int = 80) -> CommandResult:
    """Run command in a PTY with specified terminal width."""
    with tempfile.NamedTemporaryFile(delete=False) as stdout_file, \
         tempfile.NamedTemporaryFile(delete=False) as stderr_file:

        master, slave = pty.openpty()

        # Set terminal width
        winsize = struct.pack("HHHH", 24, cols, 0, 0)  # rows, cols, xpixel, ypixel
        fcntl.ioctl(slave, termios.TIOCSWINSZ, winsize)
        env = os.environ.copy()
        env["LC_ALL"] = "C"
        env["LANG"] = "C"
        env.pop("COLUMNS", None)
        env["TERM"] = "screen-256color"

        proc = subprocess.Popen(
            cmd,
            stdout=stdout_file,
            stderr=stderr_file,
            stdin=slave,
            close_fds=True,
            env=env,
        )
        os.close(slave)

        proc.wait()

    with open(stdout_file.name, 'r') as f:
        stdout_output = f.read()

    with open(stderr_file.name, 'r') as f:
        stderr_output = f.read()

    os.remove(stdout_file.name)
    os.remove(stderr_file.name)

    return CommandResult(stdout=stdout_output,
                         stderr=stderr_output,
                         returncode=proc.returncode)


if __name__ == "__main__":
    main()
