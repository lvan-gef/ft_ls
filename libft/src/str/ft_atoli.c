/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_atoli.c                                         :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/05/06 02:13:29 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:31:23 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

long int	ft_atoli(const char *str)
{
	long int	result;
	long int	index;
	int			prefix;

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
