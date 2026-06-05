/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_memmove.c                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/16 17:26:59 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:27:34 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

void *ft_memmove(void *dst, const void *src, size_t len) {
    unsigned char *dst_bytes;
    const unsigned char *src_bytes;

    dst_bytes = (unsigned char *)dst;
    src_bytes = (const unsigned char *)src;
    if (dst_bytes <= src_bytes || dst_bytes >= src_bytes + len) {
        while (len--) {
            *dst_bytes++ = *src_bytes++;
        }
        return (dst);
    }

    while (len > 0) {
        len--;
        dst_bytes[len] = src_bytes[len];
    }
    return (dst);
}
