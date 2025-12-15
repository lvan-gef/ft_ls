#include <stddef.h>

#include "../include/ft_assert.h"
#include "../include/ft_parser.h"
#include "../libft/include/ft_printf.h"
#include "../libft/include/libft.h"

static bool add_node_(t_arguments *args, const char *path);
static void print_error_(const char *flag);
static void free_path_(void *node);

bool parse_args(int argc, char **argv, t_arguments *args) {
    CUSTOM_ASSERT_(argc > 0, "argc must be more then 0");
    CUSTOM_ASSERT_(argv != NULL, "argv can not be NULL");
    CUSTOM_ASSERT_(argv[0] != NULL, "argv[0] can not be NULL");

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
                        default:
                            print_error_(argv[index]);
                            return false;
                    }
                    ++sub_index;
                }
            } else {
                if (!add_node_(args, argv[index])) {
                    return false;
                }
            }
        } else {
            if (!add_node_(args, argv[index])) {
                return false;
            }
        }

        ++index;
    }

    return true;
}

void free_args(t_arguments *args) {
    CUSTOM_ASSERT_(args != NULL, "args can not be NULL");

    ft_lstclear(&args->paths, free_path_);
}

static bool add_node_(t_arguments *args, const char *path) {
    CUSTOM_ASSERT_(args != NULL, "args can not be NULL");
    CUSTOM_ASSERT_(path != NULL, "path can not be NULL");
    CUSTOM_ASSERT_(path[0], "path[0] can not be '\\0'");

    t_node node = {0};
    node.path = ft_strdup(path);
    if (!node.path) {
        ft_printf("Failed to calloc path\n");
        return false;
    }

    t_list *new_node = ft_lstnew((void *)&node);
    if (!new_node) {
        free(node.path);
        return false;
    }

    ft_lstadd_back(&args->paths, new_node);
    return true;
}

static void print_error_(const char *flag) {
    ft_printf("ft_ls: invalid option -- %s\nusage: ft_ls "
              "[-Ralrt] [file ...]\n",
              flag);
}

static void free_path_(void *node) {
    CUSTOM_ASSERT_(node != NULL, "node can not be NULL");

    t_list *n = (t_list *)node;
    if ((char *)n->content) {
        free(n->content);
    }

    // free(n);
}
