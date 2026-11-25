#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "./ft_array.h"
#include "./ft_parse.h"
#include "./ft_str.h"
#include "./ft_walk.h"

static void init_timezone_(void);

int main(int argc, char **argv) {
    t_args args = {0};
    t_array inputs = {0};
    t_str *views = malloc((size_t)argc * sizeof(*views));
    if (!views) {
        return 1;
    }

    if (!array_init(&inputs, (size_t)argc)) {
        return 1;
    }

    if (!parse_args((size_t)argc, argv, &args, &inputs, views)) {
        free(views);
        return 1;
    }

    init_timezone_();
    const int exit_code = process(&args, &inputs);
    free(views);
    return exit_code;
}

static void init_timezone_(void) {
    if (!getenv("TZ")) {
        (void)setenv("TZ", ":/etc/localtime", 0);
    }

    tzset();
}
