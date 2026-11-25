#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "./ft_array.h"
#include "./ft_out.h"
#include "./ft_parse.h"
#include "./ft_utils.h"

static void print_error_(const char *reason, const char *flag, size_t len);

bool parse_args(size_t argc, char **argv, t_args *args, t_array *inputs) {
    bool is_flag = true;

    for (size_t index = 1; index < argc; ++index) {
        const size_t len = ft_strlen(argv[index]);
        if (is_flag && !ft_strncmp(argv[index], "--", 3)) {
            is_flag = false;
            continue;
        }

        if (is_flag && !ft_strncmp(argv[index], "--", 2)) {
            print_error_("unrecognized option ", argv[index], len);
            return false;
        }

        if (!is_flag || *argv[index] != '-' || len == 1) {
            if (!array_append(inputs, argv[index])) {
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
                default:
                    print_error_("invalid option -- ", &argv[index][sub_index],
                                 1);
                    return false;
            }
        }
    }

    if (!inputs->len) {
        static char dot[] = ".";
        if (!array_append(inputs, dot)) {
            return false;
        }
    }

    args->sort_time =
        !args->unsort && (args->time || (args->access_time && !args->list));
    return true;
}

static void print_error_(const char *reason, const char *flag, size_t len) {
    char buf[256];
    t_out out;

    out_init(&out, buf, sizeof(buf), STDERR_FILENO);
    out_str(&out, "ft_ls: ");
    out_str(&out, reason);
    out_put(&out, "'", 1);
    out_put(&out, flag, len);
    out_put(&out, "'", 1);
    out_str(&out, "\nTry 'ft_ls --help' for more information.\n");
    out_flush(&out);
}
