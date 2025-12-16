/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_atof.c                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/05/23 00:47:21 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:31:23 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

static	double	cal_decimal(const char *str, double nbr, size_t index)
{
	double	div;

	div = 10;
	if (str[index] == '.')
		index++;
	while (ft_isdigit(str[index]))
	{
		nbr = nbr + ((str[index] - '0') / div);
		div = div * 10.0;
		index++;
	}
	return (nbr);
}

double	ft_atof(const char *str)
{
	double	result;
	size_t	index;
	int		prefix;

	result = 0.0;
	index = 0;
	prefix = 0;
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
	result = cal_decimal(str, result, index);
	if (prefix < 0)
		result *= prefix;
	return (result);
}
