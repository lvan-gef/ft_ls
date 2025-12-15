#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "../include/ft_assert.h"

void log_assert_(const char *file, int line, const char *func,
                        const char *message) {
    time_t now = time(NULL);
    const struct tm *t = localtime(&now);

    if (t == NULL) {
        fprintf(stderr, "[ASSERT FAIL] 25:61:61 | %s:%d | %s() | %s\n", file,
                line, func, message);
    } else {
        fprintf(stderr, "[ASSERT FAIL] %02d:%02d:%02d | %s:%d | %s() | %s\n",
                t->tm_hour, t->tm_min, t->tm_sec, file, line, func, message);
    }

    fflush(stderr);
}
