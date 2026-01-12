#! /usr/bin/env python3

import sys
import os
import pty
import subprocess
import string
import struct
import fcntl
import termios

ALLOWED_FLAGS = ['R', 'a', 'l', 'r', 't']
DEBUG = True
TERMINAL_SIZES = [80, 100, 160, 240, 256, 512]
CORE_COUNT = os.cpu_count()
own_bin = './ft_ls'
if DEBUG:
    own_bin = f'{own_bin}_d'

def main() -> None:
    subprocess.run('make fclean', shell=True, capture_output=True)
    compile_ls()
    invalid_flags()
    subprocess.run('make fclean', shell=True, capture_output=True)

    for term_size in TERMINAL_SIZES:
        compile_ls(term_size=term_size)
        print('-' * 10, 'Test column width:', term_size, '-' * 10)
        simple_tests(term_size=term_size)
        subprocess.run('make fclean', shell=True, capture_output=True)


def compile_ls(term_size: int = 80) -> None:
    print('', '-' * 5, 'Compile with column width:', term_size, '-' * 5)
    if DEBUG:
        result = subprocess.run(f'make TERM_SIZE={term_size} debug -j {CORE_COUNT}',
                                shell=True,
                                capture_output=True)
    else:
        result = subprocess.run(f'make TERM_SIZE={term_size} -j {CORE_COUNT}',
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


def simple_tests(term_size: int = 80) -> None:
    paths = ['', '.', '..', 'src', 'include', 'tester']

    for p in paths:
        if p == '':
            ls_output = run_with_pty(cmd=['ls'], cols=term_size)
            ft_ls_output = run_with_pty(cmd=[f'{own_bin}'], cols=term_size)
        else:
            ls_output = run_with_pty(cmd=['ls', p], cols=term_size)
            ft_ls_output = run_with_pty(cmd=[f'{own_bin}', p], cols=term_size)

        try:
            assert ls_output == ft_ls_output
        except AssertionError:
            print(ls_output, file=sys.stderr)
            print('-' * 100, file=sys.stderr)
            print(ft_ls_output, file=sys.stderr)
            raise AssertionError('output ls and ft_ls are different')


def run_with_pty(cmd: list[str], cols: int = 80):
    master, slave = pty.openpty()

    # Set terminal width
    winsize = struct.pack('HHHH', 24, cols, 0, 0)  # rows, cols, xpixel, ypixel
    fcntl.ioctl(slave, termios.TIOCSWINSZ, winsize)

    proc = subprocess.Popen(
      cmd,
      stdout=slave,
      stderr=slave,
      stdin=slave,
      close_fds=True
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
    return output.decode()


if __name__ == '__main__':
    main()
