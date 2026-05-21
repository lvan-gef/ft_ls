/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_memset.c                                        :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/06 12:27:27 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 21:59:33 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

void *ft_memset(void *dest, int val, size_t len) {
    unsigned char *ptr;
    unsigned char byte;

    ptr = (unsigned char *)dest;
    byte = (unsigned char)val;
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
    return (dest);
}
