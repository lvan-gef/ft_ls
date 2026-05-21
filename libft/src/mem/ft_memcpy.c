/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_memcpy.c                                        :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/06 16:38:50 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:27:34 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

void *ft_memcpy(void *dst, const void *src, size_t len) {
    unsigned char *dst_bytes;
    const unsigned char *src_bytes;

    dst_bytes = (unsigned char *)dst;
    src_bytes = (const unsigned char *)src;
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
