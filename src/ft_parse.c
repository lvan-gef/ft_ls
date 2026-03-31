#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_parse.h"
#include "../include/ft_str.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static void print_error_(const char *flag);

t_array *parse_args(Arena *arena, uint64_t argc, char **argv, t_args *args) {
    ASSERT_GE(argc, 1);
    ASSERT_NOTNULL(argv);
    ASSERT_NOTNULL(*argv);
    ASSERT_NOTNULL(args);

    t_array *inputs = init_array(arena, ARRAY_SIZE);
    if (!inputs) {
        return NULL;
    }

    bool is_flag = true;
    for (uint64_t index = 1; index < argc; ++index) {
        const size_t len = ft_strlen(argv[index]);
        if (!ft_strncmp("--", argv[index], len)) {
            is_flag = false;
            ++index;
            continue;
        }

        if (is_flag && *argv[index] == '-') {
            for (uint64_t sub_index = 1; sub_index < len; ++sub_index) {
                switch (argv[index][sub_index]) {
                    case 'R': args->recursive = true; break;
                    case 'a': args->all = true; break;
                    case 'l': args->list = true; break;
                    case 'r': args->reverse = true; break;
                    case 't': args->time = true; break;
                    default: print_error_(argv[index]); return NULL;
                }
            }
            continue;
        }

        t_str *str = create_str(arena, argv[index]);
        if (!str) {
            return NULL;
        }

        if (!append_array(inputs, (void *)str)) {
            return NULL;
        }
    }

    if (!inputs->len) {
        t_str *str = create_str(arena, ".");
        if (!str) {
            return NULL;
        }

        if (!append_array(inputs, (void *)str)) {
            return NULL;
        }
    }

    ASSERT_GE(inputs->len, 1);
    return inputs;
}

static void print_error_(const char *flag) {
    ASSERT_NOTNULL(flag);
    ASSERT_(*flag, "*flag can not be '\\0'");

    ft_fprintf(STDERR_FILENO,
               "ft_ls: invalid option -- %s\nusage: ft_ls "
               "[-Ralrt] [file ...]\n",
               flag);
}
