/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_printf.c                                        :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2023/02/12 17:51:54 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 22:17:17 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/ft_fprintf.h"

static ssize_t check_it(va_list args, char c, int fd) {
    if (c == 'c')
        return (f_write_ascii(va_arg(args, int), fd));
    else if (c == 's')
        return (f_write_str(va_arg(args, char *), fd));
    else if (c == 'd' || c == 'i')
        return (f_format_int(va_arg(args, int), 0, 10, fd));
    else if (c == 'u')
        return (f_to_hex(va_arg(args, unsigned int), 0, 10, fd));
    else if (c == 'p')
        return (f_format_ptr(va_arg(args, void *), 0, 16, fd));
    else if (c == 'x')
        return (f_to_hex(va_arg(args, unsigned int), 0, 16, fd));
    else if (c == 'X')
        return (f_to_hex(va_arg(args, unsigned int), 1, 16, fd));
    else if (c == '%')
        return (f_write_ascii('%', fd));
    else
        return (f_write_ascii(c, fd));
    return (0);
}

int ft_fprintf(int fd, const char *s, ...) {
    va_list args;
    ssize_t result;
    ssize_t counter;

    result = 0;
    va_start(args, s);
    while (*s && result > -1) {
        if (*s == '%') {
            s++;
            if (!*s)
                break;
            counter = check_it(args, *s, fd);
        } else
            counter = f_write_ascii(*s, fd);
        if (counter < 0)
            result = counter;
        else
            result += counter;
        s++;
    }
    va_end(args);
    return ((int)result);
}
