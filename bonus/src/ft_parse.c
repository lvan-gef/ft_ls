#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_parse.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

#include "./ft_printer_helper.h"

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
                default: print_error_(argv[index]); return clean_up_(inputs);
            }
        }
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
    char buf[256];
    t_str out;

    const char cmd[] = "ft_ls: invalid option -- ";
    const char usage[] = "\nusage: ft_ls [-Ralrtgufd] [file ...]\n";
    str_init(&out, buf, sizeof(buf) - 1);
    (void)(put_mem_fd(&out, cmd, sizeof(cmd) - 1, STDERR_FILENO) &&
           put_mem_fd(&out, flag, (uint64_t)ft_strlen(flag), STDERR_FILENO) &&
           put_mem_fd(&out, usage, sizeof(usage) - 1, STDERR_FILENO) &&
           flush_fd(&out, STDERR_FILENO));
}
