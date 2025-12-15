#ifndef FT_ASSERT_H
#define FT_ASSERT_H

#include <stdlib.h> // for abort

#ifndef NDEBUG
void log_assert_(const char *file, int line, const char *func,
                 const char *message);
#endif // !NDEBUG

#ifdef NDEBUG
#define CUSTOM_ASSERT_(condition, message) ((void)0)
#else
#define CUSTOM_ASSERT_(condition, message)                                     \
    do {                                                                       \
        if (!(condition)) {                                                    \
            log_assert_(__FILE__, __LINE__, __func__, message);                \
            abort();                                                           \
        }                                                                      \
    } while (0)
#endif // !NDEBUG

#endif // !FT_ASSERT_H
