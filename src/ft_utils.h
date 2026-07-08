#ifndef FT_UTILS_H
#define FT_UTILS_H

#include <stddef.h>

void *ft_memcpy(void *dst, const void *src, size_t len);
const void *ft_memchr(const void *s, int c, size_t n);
void *ft_memset(void *dest, int val, size_t len);
int ft_strncmp(const char *s1, const char *s2, size_t n);
size_t ft_strlcpy(char *dst, const char *src, size_t dstsize);
size_t ft_strlen(const char *s);
int ft_isprint(int c);
int ft_isalpha(int c);
int ft_isdigit(int c);

#endif /* ifndef FT_UTILS_H */
