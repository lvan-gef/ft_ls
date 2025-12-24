#include <stddef.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_free.h"
#include "../include/ft_ls.h"
#include "../include/ft_parser.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static bool add_path_(t_args *args, const char *pathname);
static void print_error_(const char *flag);

bool parse_args(int argc, char **argv, t_args *args) {
    CUSTOM_ASSERT_(argc > 0, "argc must be more then 0");
    CUSTOM_ASSERT_(argv, "argv can not be NULL");
    CUSTOM_ASSERT_(argv[0], "argv[0] can not be '\\0'");

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
                        case 'R':
                            args->recursive = true;
                            break;
                        case 'a':
                            args->all = true;
                            break;
                        case 'l':
                            args->list = true;
                            break;
                        case 'r':
                            args->reverse = true;
                            break;
                        case 't':
                            args->time = true;
                            break;
                        default:
                            print_error_(argv[index]);
                            return false;
                    }
                    ++sub_index;
                }
            } else {
                if (!add_path_(args, argv[index])) {
                    return false;
                }
            }
        } else {
            if (!add_path_(args, argv[index])) {
                return false;
            }
        }

        ++index;
    }

    if (!args->paths->len) {
        return default_arg(args);
    }

    return true;
}

bool default_arg(t_args *args) {
    return add_path_(args, ".");
}

static bool add_path_(t_args *args, const char *pathname) {
    t_path *path = ft_calloc(1, sizeof(*path));
    if (!path) {
        return false;
    }

    path->files = init_array(10);
    if (!path->files) {
        free(path);
        return false;
    }

    ft_strlcpy(path->path, pathname, PATH_MAX);
    if (!append_array(args->paths, (void *)path)) {
        free_array(path->files, free_file);
        free_array(path->files, free_path);
        return false;
    }

    return true;
}

static void print_error_(const char *flag) {
    CUSTOM_ASSERT_(flag != NULL, "flag can not be NULL");
    CUSTOM_ASSERT_(*flag != '\0', "flag[0] can not be '\\0'");

    ft_fprintf(STDERR_FILENO,
               "ft_ls: invalid option -- %s\nusage: ft_ls "
               "[-Ralrt] [file ...]\n",
               flag);
}
