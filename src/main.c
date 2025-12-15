#include "../include/ft_ls.h"

int main(int argc, char **argv) {
    t_arguments args = {0};
    if (argc > 1) {
        if (!parse_args(argc, argv, &args)) {
            return 1;
        }
    }

    // main flow
    return 0;
}
