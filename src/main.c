#define C_ASSERT_IMPLEMENTATION

#include <stdint.h>
#include <unistd.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_free_list.h"
#include "../include/ft_parse.h"
#include "../include/ft_walk.h"

static void clean_program_(free_list *fl);

int main(int argc, char *argv[]) {
    t_args args = {0};
    unsigned char buffer[FL_DEFAULT_SIZE];
    free_list fl;
    fl_init(&fl, buffer, sizeof(buffer));

    t_array *inputs = parse_args(&fl, (uint64_t)argc, argv, &args);
    if (!inputs) {
        clean_program_(&fl);
        return 1;
    }

    int exit_code = 0;
    process(&args, inputs, &exit_code);

    clean_program_(&fl);
    return exit_code;
}

static void clean_program_(free_list *fl) {
    ASSERT_NOTNULL(fl);

    fl_free_all(fl);
}
