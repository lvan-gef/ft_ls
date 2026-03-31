#include "ft_fprintf.h"
#include "ft_str.h"
#include <unistd.h>
#define C_ASSERT_IMPLEMENTATION

#include <stdint.h>

#include "../include/ft_free_list.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_parse.h"
#include "../include/ft_walk.h"

static void clean_program_(free_list *arena);

int main(int argc, char *argv[]) {
    t_args args = {0};
    unsigned char buffer[1024 * 8];
    free_list fl;
    free_list_init(&fl, buffer, sizeof(buffer));

    t_array *inputs = parse_args(&fl, (uint64_t)argc, argv, &args);
    if (!inputs) {
        clean_program_(&fl);
        return 2;
    }

    int exit_code = 0;
    process(&args, inputs, &exit_code);

    clean_program_(&fl);
    return exit_code;
}

static void clean_program_(free_list *fl) {
    ASSERT_NOTNULL(fl);

    free_list_free_all(fl);
}
