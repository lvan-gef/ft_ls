/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   get_next_line.c                                    :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2023/01/20 14:27:07 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/05/05 01:42:55 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/get_next_line.h"

static	char	*new_buffer(char *buffer, size_t start_pos)
{
	char	*new_buf;
	size_t	new_len;
	size_t	i;

	if (!buffer)
		return (NULL);
	new_len = gnl_str_len(buffer) - start_pos;
	if (new_len == 0)
		return (gnl_free(NULL, buffer));
	new_buf = gnl_calloc(new_len, sizeof(char));
	if (!new_buf)
		return (gnl_free(NULL, buffer));
	i = 0;
	while (buffer[start_pos])
	{
		new_buf[i] = buffer[start_pos];
		i++;
		start_pos++;
	}
	gnl_free(NULL, buffer);
	return (new_buf);
}

static	char	*append_buffer(char *buffer, char *str)
{
	char	*new_buf;
	size_t	new_len;

	if (!buffer && !str)
		return (NULL);
	new_len = gnl_str_len(buffer) + gnl_str_len(str);
	if (new_len == 0)
		return (gnl_free(buffer, str));
	new_buf = gnl_calloc(new_len, sizeof(char));
	if (!new_buf)
		return (gnl_free(buffer, str));
	gnl_strjoin(new_buf, buffer, str, new_len);
	gnl_free(buffer, str);
	return (new_buf);
}

static char	*aline(char *buffer, size_t size)
{
	char	*new_line;

	if (!buffer)
		return (NULL);
	if (size == 0)
		return (gnl_free(buffer, NULL));
	new_line = gnl_calloc(size, sizeof(char));
	if (!new_line)
		return (NULL);
	gnl_strjoin(new_line, buffer, NULL, size);
	return (new_line);
}

static	char	*read_chunk(int fd, char *buffer)
{
	ssize_t	read_size;
	char	*str_chunk;
	size_t	newline;

	newline = 0;
	while (!newline)
	{
		str_chunk = gnl_calloc(BUFFER_SIZE, sizeof(char));
		if (!str_chunk)
			return (gnl_free(buffer, NULL));
		read_size = read(fd, str_chunk, BUFFER_SIZE);
		if (read_size < 0)
			return (gnl_free(buffer, str_chunk));
		if (read_size == 0)
		{
			gnl_free(NULL, str_chunk);
			break ;
		}
		newline = gnl_is_newline(str_chunk);
		buffer = append_buffer(buffer, str_chunk);
	}
	return (buffer);
}

char	*get_next_line(int fd)
{
	static char		*buffer = NULL;
	char			*line;
	size_t			newline_pos;

	buffer = read_chunk(fd, buffer);
	if (!buffer)
		return (NULL);
	newline_pos = gnl_is_newline(buffer);
	if (newline_pos)
	{
		line = aline(buffer, newline_pos);
		if (line)
			buffer = new_buffer(buffer, newline_pos);
		else
			buffer = gnl_free(buffer, NULL);
		return (line);
	}
	line = aline(buffer, gnl_str_len(buffer));
	buffer = gnl_free(buffer, NULL);
	return (line);
}
