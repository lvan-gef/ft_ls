#include <assert.h>
#include <stddef.h>

#include "../include/ft_ls_parser.h"
#include "../libft/include/ft_printf.h"
#include "../libft/include/libft.h"

bool parse_args(int argc, char **argv, t_arguments *args) {
    size_t index = 1;
    bool is_flag = true;

    while (index < (size_t)argc) {
        const size_t len = ft_strlen(argv[index]);
        if (ft_strncmp("--", argv[index], len) == 0) {
            is_flag = false;
            ++index;
            continue;
        }

        if (is_flag) {
            if (*argv[index] == '-') {
                size_t sub_index = 1;
                while (sub_index < len) {
                    switch (argv[index][sub_index]) {
                        case 'l':
                            args->list = true;
                            break;
                        case 'R':
                            args->recursive = true;
                            break;
                        case 'a':
                            args->all = true;
                            break;
                        case 'r':
                            args->reverse = true;
                            break;
                        case 't':
                            args->time = true;
                        default:
                            ft_printf("Not a valid flag\n");
                            return false;
                    }
                    ++sub_index;
                }
            } else {

                ft_printf("is a path??\n");
            }
        } else {
            ft_printf("is a path\n");
        }

        ++index;
    }

    ft_printf("list: %d\nrecursive: %d\nall: %d\nreverse: %d\ntime: %d\n",
              args->list, args->recursive, args->all, args->reverse,
              args->time);
    return true;
}

void free_args(t_arguments *args) {
    if (!args) {
        return;
    }
}
