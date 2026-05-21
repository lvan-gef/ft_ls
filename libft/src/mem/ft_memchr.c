/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_memchr.c                                        :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/18 12:56:25 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:27:34 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

void *ft_memchr(const void *s, int c, size_t n) {
    const unsigned char *ptr;
    unsigned char byte;

    ptr = (const unsigned char *)s;
    byte = (unsigned char)c;
    while (n >= 8) {
        if (ptr[0] == byte)
            return ((void *)(ptr + 0));
        if (ptr[1] == byte)
            return ((void *)(ptr + 1));
        if (ptr[2] == byte)
            return ((void *)(ptr + 2));
        if (ptr[3] == byte)
            return ((void *)(ptr + 3));
        if (ptr[4] == byte)
            return ((void *)(ptr + 4));
        if (ptr[5] == byte)
            return ((void *)(ptr + 5));
        if (ptr[6] == byte)
            return ((void *)(ptr + 6));
        if (ptr[7] == byte)
            return ((void *)(ptr + 7));
        ptr += 8;
        n -= 8;
    }

    while (n > 0) {
        if (*ptr == byte)
            return ((void *)ptr);
        ptr++;
        n--;
    }

    return (NULL);
}
