#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_free.h"
#include "../include/ft_parser.h"
#include "../include/ft_print.h"
#include "../include/ft_walk.h"

static void clean_program(t_args *args);

int main(int argc, char **argv) {
    t_args args = {0};
    args.paths = init_array(10);
    if (!args.paths) {
        return 1;
    }

    if (argc > 1) {
        if (!parse_args(argc, argv, &args)) {
            free_args(&args);
            return 1;
        }
    } else {
        if (!default_arg(&args)) {
            free_args(&args);
            return 1;
        }
    }

    if (!walk(&args)) {
        clean_program(&args);
        return 2;
    }

    print_ls(&args);

    clean_program(&args);
    return 0;
}

static void clean_program(t_args *args) {
    ASSERT_(args, "args can not be NULL");

    free_args(args);
}
