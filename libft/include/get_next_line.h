/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   get_next_line.h                                    :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2023/02/01 22:19:11 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 17:12:05 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef GET_NEXT_LINE_H
# define GET_NEXT_LINE_H
# include <unistd.h>
# include <stdlib.h>

# ifndef BUFFER_SIZE
#  define BUFFER_SIZE 1024
# endif

/**
* @brief Read a line from a file descriptor
*
* @param fd File descriptor to read from (int)
*
* @return char* | NULL
*/
char	*get_next_line(int fd);

/**
* @brief Free buffer and string
*
* @param buffer Buffer that must be freed (char*)
* @param str    String that must be freed (char*)
*
* @return NULL
*/
char	*gnl_free(char *buffer, char *str);

/**
* @brief Allocated a memory area and set it to zeros
*
* @param count How many objects you want (size_t)
* @param size  Size of the object        (size_t)
*
* @return void* | NULL
*/
char	*gnl_calloc(size_t count, size_t size);

/**
* @brief Check if string has a \n
*
* @param str String that must be checked (char*)
*
* @return size_t >0 | 0
*/
size_t	gnl_is_newline(const char *str);

/**
* @brief Get len of string
*
* @param s String to get len of (char*)
*
* @return size_t
*/
size_t	gnl_str_len(const char *str);

/**
* @brief Join <str1> and <str2> to <dest>
*
* @param dst  New string (char*)
* @param str1 String that must be joined (cahr*)
* @param str2 String that must be joined (cahr*)
* @param size Size of the joined string  (size_t)
*
* @return void
*/
void	gnl_strjoin(char *dst, const char *str1, const char *str2, size_t size);
#endif
