/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_memset.c                                        :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/06 12:27:27 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 21:59:33 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

void	*ft_memset(void *dest, int val, int len)
{
	unsigned char	*ptr;

	ptr = (unsigned char *)dest;
	while (len--)
	{
		*ptr++ = (unsigned char)val;
	}
	return (dest);
}
