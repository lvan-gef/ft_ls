#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_free_list.h"
#include "../include/ft_parse.h"
#include "../include/ft_str.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static t_str *create_and_append_(const t_alloc *alloc, const char *arg,
                                 t_array *inputs);
static void print_error_(const char *flag);

t_array *parse_args(free_list *fl, const uint64_t argc, char **argv,
                    t_args *args) {
    t_array *inputs = init_array(fl, ARRAY_SIZE);
    if (!inputs) {
        return NULL;
    }

    const t_alloc alloc = {.kind = ALLOC_FL, .as.fl = fl};
    bool is_flag = true;
    for (uint64_t index = 1; index < argc; ++index) {
        const size_t len = ft_strlen(argv[index]);
        if (argv[index][0] == '-' && argv[index][1] == '-' &&
            argv[index][2] == '\0') {
            is_flag = false;
            continue;
        }

        if (is_flag && *argv[index] == '-') {
            if (len == 1) {
                if (!create_and_append_(&alloc, argv[index], inputs)) {
                    return NULL;
                }
            }

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

        if (!create_and_append_(&alloc, argv[index], inputs)) {
            return NULL;
        }
    }

    if (!inputs->len) {
        if (!create_and_append_(&alloc, ".", inputs)) {
            return NULL;
        }
    }

    return inputs;
}

static t_str *create_and_append_(const t_alloc *alloc, const char *arg,
                                 t_array *inputs) {
    t_str *str = create_str(alloc, arg);
    if (!str) {
        return NULL;
    }

    if (!append_array(inputs, (void *)str)) {
        fl_free(alloc->as.fl, str);
        return NULL;
    }

    return str;
}

static void print_error_(const char *flag) {
    ft_fprintf(STDERR_FILENO,
               "ft_ls: invalid option -- %s\nusage: ft_ls "
               "[-Ralrt] [file ...]\n",
               flag);
}
