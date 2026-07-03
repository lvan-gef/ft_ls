#ifndef FT_STR_H
#define FT_STR_H

#include <stdint.h>

typedef struct s_str {
    char *str;
    uint64_t cap;
    uint64_t len;
} t_str;

void str_init(t_str *str, char *buf, uint64_t cap);
void str_copy_cstr(t_str *str, const char *src, uint64_t len);
void str_copy_uint(t_str *str, uint64_t value);
uint64_t str_uint_len(uint64_t value);
t_str *str_new(uint64_t cap);
t_str *str_from_cstr(const char *src);
t_str *str_dup(const t_str *src);
void str_free(t_str *str);
#endif /* ifndef FT_STR_H */
