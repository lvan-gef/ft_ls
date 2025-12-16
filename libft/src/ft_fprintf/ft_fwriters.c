/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_writers.c                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2023/02/12 17:42:54 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 22:17:35 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/ft_fprintf.h"
#include <stddef.h>

ssize_t f_write_str(const char *s, int fd) {
    int count;
    ssize_t result;

    count = 0;
    if (!s)
        s = "(null)";
    while (s[count]) {
        result = f_write_ascii(s[count], fd);
        if (result < 0)
            return (-1);
        count++;
    }
    return (count);
}

ssize_t f_write_ascii(int c, int fd) {
    return (write(fd, &c, 1));
}
