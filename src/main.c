#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "./ft_array.h"
#include "./ft_parse.h"
#include "./ft_str.h"
#include "./ft_walk.h"

static void del_str_(void *ptr);
static void init_timezone_(void);

int main(const int argc, const char *argv[]) {
    t_args args = {0};
    t_array inputs = {0};
    if (!array_init(&inputs, (uint64_t)argc)) {
        return 1;
    }

    if (!parse_args((uint64_t)argc, argv, &args, &inputs)) {
        array_destroy_with(&inputs, del_str_);
        return 1;
    }

    init_timezone_();
    const int exit_code = process(&args, &inputs);
    array_destroy_with(&inputs, del_str_);
    return exit_code;
}

static void del_str_(void *ptr) {
    str_free((t_str *)ptr);
}

static void init_timezone_(void) {
    if (!getenv("TZ")) {
        (void)setenv("TZ", ":/etc/localtime", 0);
    }

    tzset();
}
