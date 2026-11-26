#include <stddef.h>

#include "./ft_utils.h"

void *ft_memcpy(void *dst, const void *src, size_t len) {
    unsigned char *dst_bytes = (unsigned char *)dst;
    const unsigned char *src_bytes = (const unsigned char *)src;

    while (len > 0) {
        *dst_bytes++ = *src_bytes++;
        --len;
    }

    return dst;
}

const void *ft_memchr(const void *s, const int c, size_t n) {
    const unsigned char *ptr = (const unsigned char *)s;
    const unsigned char byte = (unsigned char)c;

    while (n > 0) {
        if (*ptr == byte) {
            return (const void *)ptr;
        }

        ptr++;
        n--;
    }

    return NULL;
}

int ft_strncmp(const char *s1, const char *s2, const size_t n) {
    const unsigned char *s1_ = (const unsigned char *)s1;
    const unsigned char *s2_ = (const unsigned char *)s2;
    size_t index = 0;

    while (index < n) {
        if (s1_[index] != s2_[index]) {
            return s1_[index] - s2_[index];
        }

        if (!s1_[index]) {
            return 0;
        }

        index++;
    }

    return 0;
}

size_t ft_strlcpy(char *dst, const char *src, const size_t dstsize) {
    const size_t src_len = ft_strlen(src);
    if (!dstsize) {
        return src_len;
    }

    size_t copy_len = src_len;
    if (copy_len >= dstsize) {
        copy_len = dstsize - 1;
    }

    ft_memcpy(dst, src, copy_len);
    dst[copy_len] = '\0';
    return src_len;
}

size_t ft_strlen(const char *s) {
    size_t index = 0;
    while (s[index]) {
        index++;
    }

    return index;
}

int ft_isprint(const int c) {
    return (c >= ' ' && c <= '~');
}

int ft_isalpha(const int c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int ft_isdigit(const int c) {
    return (c >= '0' && c <= '9');
}
