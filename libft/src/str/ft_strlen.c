/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_strlen.c                                        :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/06 14:33:47 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:40:39 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

size_t	ft_strlen(const char *s)
{
	size_t	index;

	index = 0;
	if (s == NULL)
		return (index);
	while (s[index] != '\0')
		index++;
	return (index);
}
