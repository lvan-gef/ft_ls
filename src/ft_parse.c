#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "./ft_array.h"
#include "./ft_out.h"
#include "./ft_parse.h"
#include "./ft_str.h"
#include "./ft_utils.h"

static bool append_input_(const char *arg, t_array *inputs);
static void print_error_(const char *flag);

bool parse_args(const size_t argc, const char **argv, t_args *args,
                t_array *inputs) {
    bool is_flag = true;

    for (size_t index = 1; index < argc; ++index) {
        const size_t len = ft_strlen(argv[index]);
        if (is_flag && !ft_strncmp(argv[index], "--", 2)) {
            is_flag = false;
            continue;
        }

        if (!is_flag || *argv[index] != '-' || len == 1) {
            if (!append_input_(argv[index], inputs)) {
                return false;
            }

            continue;
        }

        for (size_t sub_index = 1; sub_index < len; ++sub_index) {
            switch (argv[index][sub_index]) {
                case 'R': args->recursive = true; break;
                case 'a': args->all = true; break;
                case 'l': args->list = true; break;
                case 'r': args->reverse = true; break;
                case 't':
                    args->time = true;
                    args->unsort = false;
                    break;
                case 'g':
                    args->no_owner = true;
                    args->list = true;
                    break;
                case 'u': args->access_time = true; break;
                case 'f':
                    args->unsort = true;
                    args->all = true;
                    break;
                case 'd': args->directory = true; break;
                case 'o':
                    args->no_group = true;
                    args->list = true;
                    break;
                case 'G': args->color = true; break;
                default: print_error_(argv[index]); return false;
            }
        }
    }

    if (!inputs->len) {
        if (!append_input_(".", inputs)) {
            return false;
        }
    }

    args->sort_time =
        !args->unsort && (args->time || (args->access_time && !args->list));
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

static void print_error_(const char *flag) {
    char buf[256];
    t_out out;

    out_init(&out, buf, sizeof(buf), STDERR_FILENO);
    out_str(&out, "ft_ls: invalid option -- ");
    out_str(&out, flag);
    out_str(&out, "\nusage: ft_ls [-RalrtgufdoG] [file ...]\n");
    out_flush(&out);
}
