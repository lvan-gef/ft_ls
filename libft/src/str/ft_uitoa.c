/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_uitoa.c                                          :+:    :+:           */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/25 17:19:26 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 22:20:04 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

static size_t get_len(unsigned int c);

char *ft_uitoa(unsigned int n) {
    size_t len;
    char *str;

    len = get_len(n);
    str = ft_calloc(len + 1, sizeof(char));
    if (!str)
        return (NULL);
    if (n == 0)
        str[0] = '0';
    while (n != 0) {
        str[len] = (char)((n % 10) + '0');
        n = n / 10;
        len--;
    }
    return (str);
}

static size_t get_len(unsigned int c) {
    size_t counter;
    unsigned int base;

    counter = 0;
    base = 10;
    if (c == 0) {
        counter++;
    }
    while (c != 0) {
        c = c / base;
        counter++;
    }
    return (counter);
}
