/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_toupper.c                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/06 14:37:02 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/05/04 23:17:24 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

int	ft_toupper(int c)
{
	if (!ft_isalpha(c))
		return (c);
	if (c < 97 || c > 122)
		return (c);
	return (c - 32);
}
