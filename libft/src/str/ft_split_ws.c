/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_split_ws.c                                      :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/05/23 00:34:22 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 22:11:06 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

static	void	free_me(char **words, size_t len)
{
	size_t	index;

	index = 0;
	while (index < len)
	{
		if (words[index])
			free(words[index]);
		index++;
	}
	free(words);
}

static	size_t	word_counter(const char *s)
{
	size_t	index;
	size_t	counter;
	size_t	delim;

	index = 0;
	counter = 0;
	delim = 1;
	while (s[index] != '\0')
	{
		if (ft_iswhitespace(s[index]) != 0)
			delim = 1;
		else
		{
			if (delim)
			{
				delim = 0;
				counter++;
			}
		}
		index++;
	}
	return (counter);
}

static	int	add_me(const char *s, size_t start, size_t stop, char **words)
{
	int	index;

	index = 0;
	while (words[index])
		index++;
	words[index] = ft_substr(s, (unsigned int)start, stop - start);
	if (!words[index])
		return (0);
	return (1);
}

static	int	find_words(const char *s, char **words, size_t len)
{
	size_t	index;
	size_t	fc;

	index = 0;
	while (index < len)
	{
		while (index < len)
		{
			if (ft_iswhitespace(s[index]) != 1)
				break ;
			index++;
		}
		fc = index;
		while (index < len)
		{
			if (ft_iswhitespace(s[index]) != 0)
				break ;
			index++;
		}
		if (index > fc)
			if (!add_me(s, fc, index, words))
				return (0);
	}
	return (1);
}

char	**ft_split_ws(char const *s)
{
	size_t	wc;
	size_t	len;
	char	**words;

	if (!s)
		return (NULL);
	wc = word_counter(s) + 1;
	len = ft_strlen(s);
	words = ft_calloc(wc, sizeof(char *));
	if (!words)
		return (NULL);
	if (!find_words(s, words, len))
	{
		free_me(words, wc);
		return (NULL);
	}
	return (words);
}
