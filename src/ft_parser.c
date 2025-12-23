#include <stddef.h>
#include <unistd.h>

#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_parser.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static bool add_node_(t_args *args, const char *path);
static void print_error_(const char *flag);
static void free_path_(void *node);

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

void free_args(t_args *args) {
    CUSTOM_ASSERT_(args, "args can not be NULL");

    ft_lstclear(&args->paths, free_path_);
}

static bool add_node_(t_args *args, const char *path) {
    CUSTOM_ASSERT_(args != NULL, "args can not be NULL");
    CUSTOM_ASSERT_(path != NULL, "path can not be NULL");
    CUSTOM_ASSERT_(path[0], "path[0] can not be '\\0'");

    t_node *node = ft_calloc(1, sizeof(*node));
    if (!node) {
        ft_fprintf(STDERR_FILENO, "Failed to malloc node\n");
        return false;
    }
    ft_strlcpy(node->path, path, MAX_PATH);

    t_list *new_node = ft_lstnew((void *)node);
    if (!new_node) {
        free(node);
        return false;
    }

    ft_lstadd_back(&args->paths, new_node);
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

static void free_path_(void *content) {
    CUSTOM_ASSERT_(content != NULL, "content can not be NULL");

    t_node *node = (t_node *)content;
    free(node);
}
