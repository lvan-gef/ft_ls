#include "../include/ft_assert.h"
#include "../include/ft_parser.h"
#include "../include/ft_print.h"
#include "../include/ft_walk.h"
#include "../libft/include/libft.h"

static void clean_program(t_args *args, t_list *paths);

int main(int argc, char **argv) {
    t_args args = {0};
    if (argc > 1) {
        if (!parse_args(argc, argv, &args)) {
            free_args(&args);
            return 1;
        }
    } else {
       if (!default_arg(&args)) {
            return 1;
       }
    }

    t_list *paths = NULL;
    if (!walk(&args, &paths)) {
        clean_program(&args, paths);
        return 2;
    }

    print_ls(&args, paths);

    clean_program(&args, paths);
    return 0;
}

static void clean_program(t_args *args, t_list *paths) {
    CUSTOM_ASSERT_(args != NULL, "args can not be NULL");
    CUSTOM_ASSERT_(paths != NULL, "paths can not be NULL");

    free_args(args);
    ft_lstclear(&paths, free_walk);
}
