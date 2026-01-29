#! /usr/bin/env python3

from pathlib import Path
import difflib
import fcntl
import os
import pty
import string
import struct
import subprocess
import sys
import termios
import shutil
from itertools import combinations

from create_test_folders import create_test_folders

ALLOWED_FLAGS = ['R', 'a', 'l', 'r', 't']
DEBUG = True
TERMINAL_MIN = 80   # include
TERMINAL_MAX = 513  # exclude
own_bin = './ft_ls'
if DEBUG:
    own_bin = f'{own_bin}_d'


def main() -> None:
    test_path = Path.cwd().joinpath('ft_ls_tester')
    test_files = create_test_folders(path=test_path)

    # Phase 1: Invalid flags (no terminal width dependency)
    subprocess.run('make fclean', shell=True, capture_output=True)
    compile_ls()
    print('=' * 60)
    print('Phase 1: Invalid Flags')
    print('=' * 60)
    invalid_flags()

    # Phase 2-5: Flag combination and feature tests
    print('=' * 60)
    print('Phase 2-5: Flag Combinations and Feature Tests')
    print('=' * 60)
    flag_combination_tests(test_path)
    subprocess.run('make fclean', shell=True, capture_output=True)

    # Phase 6: Terminal width tests
    for term_size in range(TERMINAL_MIN, TERMINAL_MAX):
        compile_ls(term_size=term_size)
        print('-' * 10, 'Test column width:', term_size, '-' * 10)
        simple_tests(term_size=term_size, test_files=test_files)
        subprocess.run('make fclean', shell=True, capture_output=True)

    shutil.rmtree(test_path)
    print('=' * 60)
    print('All tests passed!')
    print('=' * 60)


def compile_ls(term_size: int = 80) -> None:
    print('', '-' * 5, 'Compile with column width:', term_size, '-' * 5)
    if DEBUG:
        result = subprocess.run(f'make TERM_SIZE={term_size} debug',
                                shell=True,
                                capture_output=True)
    else:
        result = subprocess.run(f'make TERM_SIZE={term_size}',
                                shell=True,
                                capture_output=True)

    assert result.returncode == 0, f'Compilation failed: {result.stderr.decode()}'


def invalid_flags() -> None:
    """Phase 1: Test all invalid flag characters."""
    for lttr in string.ascii_letters:
        if lttr in ALLOWED_FLAGS:
            continue

        result = subprocess.run(f'{own_bin} -{lttr}',
                                shell=True,
                                capture_output=True,
                                text=True)
        assert result.returncode == 1, f'Flag -{lttr} should return exit code 1'
        assert result.stdout == '', f'Flag -{lttr} should have empty stdout'
        assert result.stderr != '', f'Flag -{lttr} should have stderr message'

    print(f'  Tested {52} invalid flags: all passed')


def flag_combination_tests(test_path: Path) -> None:
    """Phase 2-5: Test all flag combinations against test fixtures."""
    # Generate all flag combinations
    flag_combos = generate_flag_combinations()

    # Test directories to use
    test_dirs = [
        test_path / 'simple',
        test_path / 'hidden',
        test_path / 'timestamps',
        test_path / 'recursive',
        test_path / 'symlinks',
        test_path / 'permissions',
        test_path / 'sizes',
        test_path / 'special_chars',
        test_path / 'empty',
        test_path / 'sort_test',
    ]

    # Phase 2: Single flag tests
    print('\n  Phase 2: Single Flag Tests')
    single_flags = ['', '-R', '-a', '-l', '-r', '-t']
    for flags in single_flags:
        for test_dir in test_dirs:
            compare_output(flags, str(test_dir))
    print(f'    Tested {len(single_flags)} flag variants x {len(test_dirs)} dirs')

    # Phase 3: Flag combinations (2-5 flags)
    print('\n  Phase 3: Flag Combination Tests')
    combo_count = 0
    for flags in flag_combos:
        for test_dir in test_dirs:
            compare_output(flags, str(test_dir))
            combo_count += 1
    print(f'    Tested {len(flag_combos)} combinations x {len(test_dirs)} dirs')

    # Phase 4: Feature-specific tests
    print('\n  Phase 4: Feature-Specific Tests')
    feature_specific_tests(test_path)

    # Phase 5: Edge case tests
    print('\n  Phase 5: Edge Case Tests')
    edge_case_tests(test_path)


def generate_flag_combinations() -> list[str]:
    """Generate all meaningful flag combinations."""
    flags = ALLOWED_FLAGS.copy()
    combos = []

    # 2-flag combinations
    for r in range(2, len(flags) + 1):
        for combo in combinations(flags, r):
            combos.append('-' + ''.join(combo))

    return combos


def feature_specific_tests(test_path: Path) -> None:
    """Phase 4: Test specific features in detail."""

    # 4.1 Hidden files (-a) test
    hidden_dir = test_path / 'hidden'
    # Without -a: should not show hidden files
    output_no_a = run_with_pty(cmd=[own_bin, str(hidden_dir)])
    assert '.hidden1' not in output_no_a, 'Hidden files should not appear without -a'
    assert 'visible1.txt' in output_no_a, 'Visible files should appear without -a'

    # With -a: should show hidden files
    output_with_a = run_with_pty(cmd=[own_bin, '-a', str(hidden_dir)])
    assert '.hidden1' in output_with_a, 'Hidden files should appear with -a'
    assert '.' in output_with_a, '. should appear with -a'
    assert '..' in output_with_a, '.. should appear with -a'
    print('    4.1 Hidden files (-a): passed')

    # 4.2 Time sorting (-t) test
    ts_dir = test_path / 'timestamps'
    output_t = run_with_pty(cmd=[own_bin, '-t', str(ts_dir)])
    ls_output_t = run_with_pty(cmd=['ls', '-t', str(ts_dir)])
    assert output_t == ls_output_t, 'Time sorting should match ls -t'
    print('    4.2 Time sorting (-t): passed')

    # 4.3 Reverse sorting (-r) test
    simple_dir = test_path / 'simple'
    output_r = run_with_pty(cmd=[own_bin, '-r', str(simple_dir)])
    ls_output_r = run_with_pty(cmd=['ls', '-r', str(simple_dir)])
    assert output_r == ls_output_r, 'Reverse sorting should match ls -r'
    print('    4.3 Reverse sorting (-r): passed')

    # 4.4 Recursive listing (-R) test
    rec_dir = test_path / 'recursive'
    output_R = run_with_pty(cmd=[own_bin, '-R', str(rec_dir)])
    ls_output_R = run_with_pty(cmd=['ls', '-R', str(rec_dir)])
    assert output_R == ls_output_R, 'Recursive listing should match ls -R'
    # Verify all levels are traversed
    assert 'level1_a' in output_R, 'level1_a should appear in -R output'
    assert 'level2_a' in output_R, 'level2_a should appear in -R output'
    assert 'level3_a' in output_R or 'deepest.txt' in output_R, \
        'Deepest level should appear in -R output'
    print('    4.4 Recursive listing (-R): passed')

    # 4.5 Long format (-l) test
    perm_dir = test_path / 'permissions'
    output_l = run_with_pty(cmd=[own_bin, '-l', str(perm_dir)])
    ls_output_l = run_with_pty(cmd=['ls', '-l', str(perm_dir)])
    assert output_l == ls_output_l, 'Long format should match ls -l'
    # Verify permission columns appear
    assert 'rw' in output_l, 'Permission string should appear in -l output'
    print('    4.5 Long format (-l): passed')

    # 4.6 Recursive time sort (-Rt) test
    rec_dir = test_path / 'recursive'
    output_Rt = run_with_pty(cmd=[own_bin, '-Rt', str(rec_dir)])
    ls_output_Rt = run_with_pty(cmd=['ls', '-Rt', str(rec_dir)])
    assert output_Rt == ls_output_Rt, 'Recursive time sort should match ls -Rt'
    print('    4.6 Recursive time sort (-Rt): passed')


def edge_case_tests(test_path: Path) -> None:
    """Phase 5: Test edge cases."""

    # 5.1 Symlinks test
    sym_dir = test_path / 'symlinks'
    output_l = run_with_pty(cmd=[own_bin, '-l', str(sym_dir)])
    ls_output_l = run_with_pty(cmd=['ls', '-l', str(sym_dir)])
    assert output_l == ls_output_l, 'Symlink listing should match ls -l'
    assert 'link_to_file' in output_l, 'Symlinks should be listed'
    print('    5.1 Symlinks: passed')

    # 5.2 Special characters test
    special_dir = test_path / 'special_chars'
    output = run_with_pty(cmd=[own_bin, str(special_dir)])
    ls_output = run_with_pty(cmd=['ls', str(special_dir)])
    assert output == ls_output, 'Special char filenames should match ls'
    print('    5.2 Special characters: passed')

    # 5.3 Empty directory test
    empty_dir = test_path / 'empty'
    output = run_with_pty(cmd=[own_bin, str(empty_dir)])
    ls_output = run_with_pty(cmd=['ls', str(empty_dir)])
    assert output == ls_output, 'Empty directory output should match ls'

    output_a = run_with_pty(cmd=[own_bin, '-a', str(empty_dir)])
    ls_output_a = run_with_pty(cmd=['ls', '-a', str(empty_dir)])
    assert output_a == ls_output_a, 'Empty dir with -a should match ls -a'
    print('    5.3 Empty directory: passed')

    # 5.4 Multiple paths test
    dir_a = test_path / 'multi_path' / 'dir_a'
    dir_b = test_path / 'multi_path' / 'dir_b'
    output = run_with_pty(cmd=[own_bin, str(dir_a), str(dir_b)])
    ls_output = run_with_pty(cmd=['ls', str(dir_a), str(dir_b)])
    assert output == ls_output, 'Multiple path output should match ls'
    # Verify both directories have headers
    assert 'dir_a' in output or 'a1.txt' in output, 'dir_a content should appear'
    assert 'dir_b' in output or 'b1.txt' in output, 'dir_b content should appear'
    print('    5.4 Multiple paths: passed')

    # 5.5 Non-existent path test
    result_ft = subprocess.run([own_bin, '/nonexistent/path'],
                               capture_output=True, text=True)
    result_ls = subprocess.run(['ls', '/nonexistent/path'],
                               capture_output=True, text=True)
    assert result_ft.returncode == result_ls.returncode, \
        'Non-existent path return code should match ls'
    assert result_ft.stderr != '', 'Non-existent path should produce stderr'
    print('    5.5 Non-existent path: passed')

    # 5.6 File argument test (ls can take a file as argument)
    file_path = test_path / 'simple' / 'abc.txt'
    output = run_with_pty(cmd=[own_bin, str(file_path)])
    ls_output = run_with_pty(cmd=['ls', str(file_path)])
    assert output == ls_output, 'File argument output should match ls'
    print('    5.6 File argument: passed')

    # 5.7 Sort test directory
    sort_dir = test_path / 'sort_test'
    for flags in ['', '-r', '-a', '-ar']:
        flag_list = [own_bin] + ([flags] if flags else []) + [str(sort_dir)]
        ls_flag_list = ['ls'] + ([flags] if flags else []) + [str(sort_dir)]
        output = run_with_pty(cmd=flag_list)
        ls_output = run_with_pty(cmd=ls_flag_list)
        assert output == ls_output, f'Sort test with {flags or "no flags"} should match'
    print('    5.7 Sort edge cases: passed')

    # 5.8 Long format with time sort (-lt) on sizes
    sizes_dir = test_path / 'sizes'
    output_lt = run_with_pty(cmd=[own_bin, '-lt', str(sizes_dir)])
    ls_output_lt = run_with_pty(cmd=['ls', '-lt', str(sizes_dir)])
    assert output_lt == ls_output_lt, 'Long format with time sort should match ls -lt'
    print('    5.8 Long format time sort (-lt): passed')

    # 5.9 All flags combined (-Rlatr) on recursive
    rec_dir = test_path / 'recursive'
    output_all = run_with_pty(cmd=[own_bin, '-Rlatr', str(rec_dir)])
    ls_output_all = run_with_pty(cmd=['ls', '-Rlatr', str(rec_dir)])
    assert output_all == ls_output_all, 'All flags combined should match ls -Rlatr'
    print('    5.9 All flags combined (-Rlatr): passed')


def compare_output(flags: str, path: str, cols: int = 80) -> None:
    """Compare ft_ls output with system ls output."""
    if flags:
        ft_cmd = [own_bin, flags, path]
        ls_cmd = ['ls', flags, path]
    else:
        ft_cmd = [own_bin, path]
        ls_cmd = ['ls', path]

    ft_output = run_with_pty(cmd=ft_cmd, cols=cols)
    ls_output = run_with_pty(cmd=ls_cmd, cols=cols)

    if ft_output != ls_output:
        print(f'\nMismatch for: {" ".join(ft_cmd)}', file=sys.stderr)
        print(f'ls output:\n{ls_output}', file=sys.stderr)
        print('-' * 40, file=sys.stderr)
        print(f'ft_ls output:\n{ft_output}', file=sys.stderr)
        print('-' * 40, file=sys.stderr)

        # Show diff
        seqm = difflib.SequenceMatcher(None, ft_output, ls_output)
        for opcode, a0, a1, b0, b1 in seqm.get_opcodes():
            if opcode == 'replace':
                print(f"Replace '{repr(ft_output[a0:a1])}' with "
                      f"'{repr(ls_output[b0:b1])}'", file=sys.stderr)
            elif opcode == 'insert':
                print(f"Insert '{repr(ls_output[b0:b1])}'", file=sys.stderr)
            elif opcode == 'delete':
                print(f"Delete '{repr(ft_output[a0:a1])}'", file=sys.stderr)

        raise AssertionError(f'Output mismatch for: {" ".join(ft_cmd)}')


def simple_tests(term_size: int, test_files: list[Path]) -> None:
    """Phase 6: Terminal width tests across all fixtures."""
    paths = ['', '.', '..', 'src', 'include', 'tester']
    for p in test_files:
        paths.append(str(p))

    for p in paths:
        if p == '':
            ls_output = run_with_pty(cmd=['ls'], cols=term_size)
            ft_ls_output = run_with_pty(cmd=[own_bin], cols=term_size)
        else:
            ls_output = run_with_pty(cmd=['ls', p], cols=term_size)
            ft_ls_output = run_with_pty(cmd=[own_bin, p], cols=term_size)

        if ls_output != ft_ls_output:
            print(f'Path: {p}', file=sys.stderr)
            print(f'ls:\n{ls_output}', file=sys.stderr)
            print('-' * term_size, file=sys.stderr)
            print(f'ft_ls:\n{ft_ls_output}', file=sys.stderr)
            print('-' * term_size, file=sys.stderr)
            seqm = difflib.SequenceMatcher(None, ft_ls_output, ls_output)
            for opcode, a0, a1, b0, b1 in seqm.get_opcodes():
                if opcode == 'replace':
                    print(f"Replace '{repr(ft_ls_output[a0:a1])}' with "
                          f"'{repr(ls_output[b0:b1])}'", file=sys.stderr)
                elif opcode == 'insert':
                    print(f"Insert '{repr(ls_output[b0:b1])}'", file=sys.stderr)
                elif opcode == 'delete':
                    print(f"Delete '{repr(ft_ls_output[a0:a1])}'", file=sys.stderr)
            raise AssertionError(f'Output mismatch for path: {p}')


def run_with_pty(cmd: list[str], cols: int = 80) -> str:
    """Run command in a PTY with specified terminal width."""
    master, slave = pty.openpty()

    # Set terminal width
    winsize = struct.pack('HHHH', 24, cols, 0, 0)  # rows, cols, xpixel, ypixel
    fcntl.ioctl(slave, termios.TIOCSWINSZ, winsize)
    env = os.environ.copy()
    env.pop('COLUMNS', None)
    env['TERM'] = 'screen-256color'

    proc = subprocess.Popen(
        cmd,
        stdout=slave,
        stderr=subprocess.DEVNULL,
        stdin=slave,
        close_fds=True,
        env=env
    )
    os.close(slave)

    output = b''
    while True:
        try:
            data = os.read(master, 1024)
            if not data:
                break
            output += data
        except OSError:
            break

    os.close(master)
    proc.wait()
    return output.decode().replace('\r', '')


if __name__ == '__main__':
    main()
