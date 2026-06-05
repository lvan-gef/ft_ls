/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_strlcpy.c                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/11 18:13:19 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:40:39 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

size_t ft_strlcpy(char *dst, const char *src, size_t dstsize) {
    size_t src_len;
    size_t copy_len;

    src_len = ft_strlen(src);
    if (!dstsize) {
        return (src_len);
    }

    copy_len = src_len;
    if (copy_len >= dstsize) {
        copy_len = dstsize - 1;
    }

    ft_memcpy(dst, src, copy_len);
    dst[copy_len] = '\0';
    return (src_len);
}
