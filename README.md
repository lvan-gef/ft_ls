# ft_ls

`ft_ls` is a C implementation of the Unix `ls` command, built as part of the 42
school curriculum. The goal of the project is to interact directly with the file
system, read directory entries, inspect file metadata, reproduce long-format
listing output, and handle recursive traversal and sorting options.

This implementation targets Linux only.

## Features

- Reimplementation of the core `ls` behavior.
- Long-format listing with permissions, links, owner, group, size, date, and symlink targets.
- Recursive directory traversal.
- Name, time, and reverse sorting.
- Hidden file handling.
- Multi-column output when `-l` is not used.
- Terminal-width aware column layout.
- ACL and extended-attribute markers in long-format permissions.
- ANSI color output through `-G`.
- Buffered output and iterative merge sort for large listings.

## Supported Options

| Option | Description |
| --- | --- |
| `-l` | Use long listing format. |
| `-R` | List subdirectories recursively. |
| `-a` | Include entries whose names begin with `.`. |
| `-r` | Reverse the selected sort order. |
| `-t` | Sort by modification time, newest first. |
| `-u` | Use access time instead of modification time where applicable. |
| `-f` | Do not sort entries and imply `-a`. |
| `-g` | Long listing format without the owner column. |
| `-o` | Long listing format without the group column. |
| `-d` | List directories themselves instead of their contents. |
| `-G` | Enable colored output. |

## Linux Note About `-G`

This project is Linux-only, but its `-G` option is used for color output as a
bonus feature inspired by the subject. This intentionally differs from GNU/Linux
`ls`, where color is usually controlled with `--color` and `-G` has a different
meaning. In this project:

```sh
./ft_ls -G
```

prints colored file names using ANSI escape sequences.

## Build

```sh
make
```

The executable is created as:

```sh
./ft_ls
```

Useful Makefile targets:

| Target | Description |
| --- | --- |
| `make` | Build the release binary. |
| `make clean` | Remove object files. |
| `make fclean` | Remove object files and the binary. |
| `make re` | Clean and rebuild. |
| `make help` | Show available Makefile targets. |

## Usage

```sh
./ft_ls [options] [file ...]
```

If no file or directory is given, `ft_ls` lists the current directory.

Examples:

```sh
./ft_ls
./ft_ls -la
./ft_ls -lRt src
./ft_ls -G
./ft_ls -d */
```

## Limitations

- This is not a complete GNU `ls` clone.
- The implementation is intended for Linux systems.
- `-G` is color output in this project and does not match GNU/Linux `ls -G`.
- Locale-specific behavior is not supported.
