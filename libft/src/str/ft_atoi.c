/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_atoi.c                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/18 13:47:29 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:31:23 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

int	ft_atoi(const char *str)
{
	int	result;
	int	prefix;
	int	index;

	result = 0;
	prefix = 0;
	index = 0;
	while (ft_strchr(" \t\v\f\r\n", str[index]))
		index++;
	if (str[index] == '-' || str[index] == '+')
	{
		if (str[index] == '-')
			prefix = -1;
		index++;
	}
	while (ft_isdigit(str[index]))
	{
		result = (result * 10) + (str[index] - '0');
		index++;
	}
	if (prefix < 0)
		result *= prefix;
	return (result);
}
