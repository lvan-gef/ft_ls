/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_atoui.c                                         :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/05/06 02:13:29 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:31:23 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"
#include <stdint.h>

uint64_t ft_atoui(const char *str) {
    uint64_t result;
    uint64_t index;

    result = 0;
    index = 0;
    while (ft_strchr(" \t\v\f\r\n", str[index]))
        index++;
    while (ft_isdigit(str[index])) {
        result = (result * 10) + (uint64_t)(str[index] - '0');
        index++;
    }
    return (result);
}
