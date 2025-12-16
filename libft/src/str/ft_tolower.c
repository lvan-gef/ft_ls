/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_tolower.c                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/06 14:36:17 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/05/04 23:17:24 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

int	ft_tolower(int c)
{
	if (!ft_isalpha(c))
		return (c);
	if (c < 65 || c > 90)
		return (c);
	return (c + 32);
}
