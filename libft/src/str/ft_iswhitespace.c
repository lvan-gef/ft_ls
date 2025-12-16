/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_iswhitespace.c                                  :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/05/23 00:29:19 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:33:38 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

int	ft_iswhitespace(int c)
{
	if ((c >= 9 && c <= 13) || c == 32)
	{
		return (1);
	}
	return (0);
}
