#include <stddef.h>

#include "../include/ft_helpers.h"

static size_t get_len(size_t c, size_t base);

size_t uitoa(char *buffer, size_t buffer_len, size_t n) {
    size_t base = 10;
    size_t len = get_len(n, base);

    if (len > buffer_len - 1) {
        len = buffer_len - 1;
    }

    size_t counter = 0;
    if (n == 0) {
        buffer[0] = '0';
        ++counter;
    }

    while (n) {
        buffer[len - 1] = (char)((n % 10) + '0');
        n = n / 10;
        --len;
        ++counter;
    }

    return counter;
}

static size_t get_len(size_t c, size_t base) {
    size_t counter = 0;

    if (c == 0) {
        ++counter;
    }

    while (c) {
        c = c / base;
        ++counter;
    }

    return counter;
}
