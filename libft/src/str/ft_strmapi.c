/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_strmapi.c                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/31 22:20:28 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 22:09:14 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

char	*ft_strmapi(char const *s, char (*f)(unsigned int, char))
{
	unsigned int	index;
	size_t			str_len;
	char			*str;

	if (!s)
		return (NULL);
	index = 0;
	str_len = ft_strlen(s);
	str = ft_calloc(str_len + 1, sizeof(char));
	if (!str)
		return (NULL);
	while (s[index] != '\0')
	{
		str[index] = f(index, s[index]);
		index++;
	}
	return (str);
}
