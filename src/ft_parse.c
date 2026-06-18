#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_parse.h"
#include "../include/ft_str.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static bool append_input_(const char *arg, t_array *inputs);
static bool clean_up_(t_array *inputs);
static void del_str_(void *ptr);
static void print_error_(const char *flag);

bool parse_args(const uint64_t argc, char **argv, t_args *args,
                t_array *inputs) {
    bool is_flag = true;

    for (uint64_t index = 1; index < argc; ++index) {
        const size_t len = ft_strlen(argv[index]);
        if (is_flag && argv[index][0] == '-' && argv[index][1] == '-' &&
            argv[index][2] == '\0') {
            is_flag = false;
            continue;
        }

        if (!is_flag || *argv[index] != '-' || len == 1) {
            if (!append_input_(argv[index], inputs)) {
                return clean_up_(inputs);
            }

            continue;
        }

        for (size_t sub_index = 1; sub_index < len; ++sub_index) {
            switch (argv[index][sub_index]) {
                case 'R': args->recursive = true; break;
                case 'a': args->all = true; break;
                case 'l': args->list = true; break;
                case 'r': args->reverse = true; break;
                case 't': args->time = true; break;
                default: print_error_(argv[index]); return clean_up_(inputs);
            }
        }
        continue;
    }

    if (!inputs->len) {
        if (!append_input_(".", inputs)) {
            return clean_up_(inputs);
        }
    }

    return true;
}

static bool append_input_(const char *arg, t_array *inputs) {
    t_str *str = str_from_cstr(arg);
    if (!str) {
        return false;
    }

    if (!array_append(inputs, (void *)str)) {
        str_free(str);
        return false;
    }

    return true;
}

static bool clean_up_(t_array *inputs) {
    array_clear_with(inputs, del_str_);
    return false;
}

static void del_str_(void *ptr) {
    str_free((t_str *)ptr);
}

static void print_error_(const char *flag) {
    ft_fprintf(STDERR_FILENO,
               "ft_ls: invalid option -- %s\nusage: ft_ls "
               "[-Ralrt] [file ...]\n",
               flag);
}
