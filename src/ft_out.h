#ifndef FT_OUT_H
#define FT_OUT_H

#include <stdbool.h>
#include <stddef.h>

typedef struct s_out {
    char *buf;
    size_t cap;
    size_t len;
    int fd;
    bool failed;
} t_out;

void out_init(t_out *out, char *buf, size_t cap, int fd);
void out_put(t_out *out, const char *src, size_t len);
void out_str(t_out *out, const char *str);
void out_pad(t_out *out, char c, size_t count);
void out_flush(t_out *out);

#endif /* ifndef FT_OUT_H */
