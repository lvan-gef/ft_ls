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

from create_test_folders import create_test_folders

# TODO: you can give ls a file then it will print that...

ALLOWED_FLAGS = ['R', 'a', 'l', 'r', 't']
DEBUG = True
TERMINAL_SIZES = [181]
# TERMINAL_SIZES = [80, 100, 160, 240, 256, 512]
own_bin = './ft_ls'
if DEBUG:
    own_bin = f'{own_bin}_d'

def main() -> None:
    test_path = Path.cwd().joinpath('ft_ls_tester')
    tests_files = create_test_folders(path=test_path)

    subprocess.run('make fclean', shell=True, capture_output=True)
    compile_ls()
    invalid_flags()
    subprocess.run('make fclean', shell=True, capture_output=True)

    for term_size in TERMINAL_SIZES:
        compile_ls(term_size=term_size)
        print('-' * 10, 'Test column width:', term_size, '-' * 10)
        simple_tests(term_size=term_size, test_files=tests_files)
        subprocess.run('make fclean', shell=True, capture_output=True)

    shutil.rmtree(test_path)

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

    assert result.returncode == 0


def invalid_flags() -> None:
    for lttr in string.ascii_letters:
        if lttr in ALLOWED_FLAGS:
            continue

        result = subprocess.run(f'{own_bin} -{lttr}', shell=True, capture_output=True, text=True)
        assert result.returncode == 1
        assert result.stdout == ''
        assert result.stderr != ''


def simple_tests(term_size, test_files: list[Path]) -> None:
    # paths = ['', '.', '..', 'src', 'include', 'tester']

    paths = ['src', 'include']
    for p in test_files:
        paths.append(str(p))


    for p in paths:
    # for p in paths[-3:-1]:
    # for p in test_files:
        # p = str(p)
        if p == '':
            ls_output = run_with_pty(cmd=['ls'], cols=term_size)
            ft_ls_output = run_with_pty(cmd=[f'{own_bin}'], cols=term_size)
        else:
            ls_output = run_with_pty(cmd=['ls', p], cols=term_size)
            ft_ls_output = run_with_pty(cmd=[f'{own_bin}', p], cols=term_size)

        try:
            assert ls_output == ft_ls_output
        except AssertionError:
            print(f'ls:\n{ls_output}', file=sys.stderr)
            print('-' * term_size, file=sys.stderr)
            print(f'ft_ls:\n{ft_ls_output}', file=sys.stderr)
            print('-' * term_size, file=sys.stderr)
            seqm = difflib.SequenceMatcher(None, ft_ls_output, ls_output)
            for opcode, a0, a1, b0, b1 in seqm.get_opcodes():
                if opcode == 'replace':
                    print(f"Replace '{ft_ls_output[a0:a1]}' with '{ls_output[b0:b1]}'")
                elif opcode == 'insert':
                    print(f"Insert '{ls_output[b0:b1]}'")
                elif opcode == 'delete':
                    print(f"Delete '{ft_ls_output[a0:a1]}'")
            # raise AssertionError('output ls and ft_ls are different')
            print('-' * term_size, file=sys.stderr)


def run_with_pty(cmd: list[str], cols: int = 80):
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
