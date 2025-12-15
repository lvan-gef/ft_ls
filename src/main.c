#include "../include/ft_parser.h"

int main(int argc, char **argv) {
    t_arguments args = {0};
    if (argc > 1) {
        if (!parse_args(argc, argv, &args)) {
            free_args(&args);
            return 1;
        }
    }

    // main flow

    free_args(&args);
    return 0;
}
