#include <stddef.h>

#include "./ft_utils.h"

void *ft_memcpy(void *dst, const void *src, size_t len) {
    unsigned char *dst_bytes = (unsigned char *)dst;
    const unsigned char *src_bytes = (const unsigned char *)src;

    while (len >= 8) {
        dst_bytes[0] = src_bytes[0];
        dst_bytes[1] = src_bytes[1];
        dst_bytes[2] = src_bytes[2];
        dst_bytes[3] = src_bytes[3];
        dst_bytes[4] = src_bytes[4];
        dst_bytes[5] = src_bytes[5];
        dst_bytes[6] = src_bytes[6];
        dst_bytes[7] = src_bytes[7];
        dst_bytes += 8;
        src_bytes += 8;
        len -= 8;
    }

    while (len > 0) {
        *dst_bytes++ = *src_bytes++;
        --len;
    }

    return dst;
}

const void *ft_memchr(const void *s, const int c, size_t n) {
    const unsigned char *ptr = (const unsigned char *)s;
    const unsigned char byte = (unsigned char)c;

    while (n >= 8) {
        if (ptr[0] == byte) {
            return ptr + 0;
        }

        if (ptr[1] == byte) {
            return ptr + 1;
        }

        if (ptr[2] == byte) {
            return ptr + 2;
        }

        if (ptr[3] == byte) {
            return ptr + 3;
        }

        if (ptr[4] == byte) {
            return ptr + 4;
        }

        if (ptr[5] == byte) {
            return ptr + 5;
        }

        if (ptr[6] == byte) {
            return ptr + 6;
        }

        if (ptr[7] == byte) {
            return ptr + 7;
        }

        ptr += 8;
        n -= 8;
    }

    while (n > 0) {
        if (*ptr == byte) {
            return ptr;
        }

        ptr++;
        n--;
    }

    return NULL;
}

void *ft_memset(void *dest, const int val, size_t len) {
    unsigned char *ptr = (unsigned char *)dest;
    const unsigned char byte = (unsigned char)val;

    while (len >= 8) {
        ptr[0] = byte;
        ptr[1] = byte;
        ptr[2] = byte;
        ptr[3] = byte;
        ptr[4] = byte;
        ptr[5] = byte;
        ptr[6] = byte;
        ptr[7] = byte;
        ptr += 8;
        len -= 8;
    }

    while (len--) {
        *ptr++ = byte;
    }

    return dest;
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
    if (c >= 32 && c <= 126) {
        return 1;
    }

    return 0;
}

int ft_isalpha(const int c) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        return 1;
    }

    return 0;
}

int ft_isdigit(const int c) {
    if (c >= 48 && c <= 57) {
        return 1;
    }

    return 0;
}
