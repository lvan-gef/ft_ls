/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_strchr.c                                        :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/16 16:39:39 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 22:01:41 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

char	*ft_strchr(const char *s, int c)
{
	char	*str;
	int		index;
	char	target;

	index = 0;
	target = (char)c;
	str = (char *)s;
	if (target == '\0')
		return (&str[ft_strlen(str)]);
	while (s[index])
	{
		if (str[index] == target)
			return (&str[index]);
		index++;
	}
	return (NULL);
}
