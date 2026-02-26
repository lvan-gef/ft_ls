#ifndef C_ASSERT_H
#define C_ASSERT_H

#include <stdlib.h>

#ifndef NDEBUG

#if defined(__GNUC__) || defined(__clang__)
#define C_PRINTF_LIKE(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define C_PRINTF_LIKE(fmt, args)
#endif

C_PRINTF_LIKE(4, 5)
void log_assert_(const char *file, int line, const char *func, const char *fmt,
                 ...);

#endif // !NDEBUG

#ifdef NDEBUG
#define ASSERT_(condition, ...) ((void)0)
#define ASSERT_NOTNULL(ptr) ((void)0)
#define ASSERT_NULL(ptr) ((void)0)
#define ASSERT_EQ(a, b) ((void)0)
#define ASSERT_NE(a, b) ((void)0)
#define ASSERT_LT(a, b) ((void)0)
#define ASSERT_LE(a, b) ((void)0)
#define ASSERT_GT(a, b) ((void)0)
#define ASSERT_GE(a, b) ((void)0)
#define ASSERT_TRUE(condition) ((void)0)
#define ASSERT_FALSE(condition) ((void)0)

#else // !NDEBUG

#define ASSERT_(condition, ...)                                                \
    do {                                                                       \
        if (!(condition)) {                                                    \
            log_assert_(__FILE__, __LINE__, __func__, __VA_ARGS__);            \
            abort();                                                           \
        }                                                                      \
    } while (0)

#define ASSERT_NOTNULL(ptr)                                                    \
    ASSERT_((ptr) != NULL, "Expected non-NULL pointer, but got NULL: " #ptr)

#define ASSERT_NULL(ptr)                                                       \
    ASSERT_((ptr) == NULL, "Expected NULL pointer, but got %p: " #ptr,         \
            (void *)(ptr))

#define ASSERT_EQ(a, b)                                                        \
    ASSERT_((a) == (b), "Expected " #a " == " #b ", but %lld != %lld",         \
            (long long)(a), (long long)(b))

#define ASSERT_NE(a, b)                                                        \
    ASSERT_((a) != (b), "Expected " #a " != " #b ", but both are %lld",        \
            (long long)(a))

#define ASSERT_LT(a, b)                                                        \
    ASSERT_((a) < (b), "Expected " #a " < " #b ", but %lld >= %lld",           \
            (long long)(a), (long long)(b))

#define ASSERT_LE(a, b)                                                        \
    ASSERT_((a) <= (b), "Expected " #a " <= " #b ", but %lld > %lld",          \
            (long long)(a), (long long)(b))

#define ASSERT_GT(a, b)                                                        \
    ASSERT_((a) > (b), "Expected " #a " > " #b ", but %lld <= %lld",           \
            (long long)(a), (long long)(b))

#define ASSERT_GE(a, b)                                                        \
    ASSERT_((a) >= (b), "Expected " #a " >= " #b ", but %lld < %lld",          \
            (long long)(a), (long long)(b))

#define ASSERT_TRUE(condition) ASSERT_(condition, "Expected true: " #condition)

#define ASSERT_FALSE(condition)                                                \
    ASSERT_(!(condition), "Expected false: " #condition)

#endif // !NDEBUG

#ifdef C_ASSERT_IMPLEMENTATION

#ifndef NDEBUG

#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void log_assert_(const char *file, int line, const char *func, const char *fmt,
                 ...) {
    time_t now = time(NULL);
    const struct tm *t = localtime(&now);

    if (t == NULL) {
        fprintf(stderr, "[ASSERT FAIL] 25:61:61 | %s:%d | %s() | ", file, line,
                func);
    } else {
        fprintf(stderr, "[ASSERT FAIL] %02d:%02d:%02d | %s:%d | %s() | ",
                t->tm_hour, t->tm_min, t->tm_sec, file, line, func);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    {
        void *buffer[16];
        int nptrs = backtrace(buffer, 16);
        backtrace_symbols_fd(buffer, nptrs, fileno(stderr));
    }

    fflush(stderr);
}

#endif // !NDEBUG

#endif // C_ASSERT_IMPLEMENTATION

#endif // C_ASSERT_H
