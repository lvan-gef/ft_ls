#! /usr/bin/env python3

import subprocess
import subprocess
import string

ALLOWED_FLAGS = ['R', 'a', 'l', 'r', 't']

def main() -> None:
    compile_ls()
    invalid_flags()


def compile_ls() -> None:
    result = subprocess.run('make debug', shell=True, capture_output=True)
    assert result.returncode == 0


def invalid_flags() -> None:
    for lttr in string.ascii_letters:
        if lttr in ALLOWED_FLAGS:
            continue

        print(lttr)
        result = subprocess.run(f'./ft_ls {lttr}', shell=True, capture_output=True)
        try:
            assert result.returncode == 1
            assert result.stdout == ''
            assert result.stderr != ''
        except AssertionError as ae:
            print(f'{ae}\nreturncode: {result.returncode}\nstdout: {result.stdout}\nstderr: {result.stderr}')


if __name__ == '__main__':
    main()
