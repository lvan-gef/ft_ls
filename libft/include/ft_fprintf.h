/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_printf.h                                        :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2023/02/12 17:51:54 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 22:18:51 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_FPRINTF_H
#define FT_FPRINTF_H

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @brief Print A number to stdout
 *
 * @param size  Number to be printed                 (unsigned llong int)
 * @param upper Is it uppercase or lowercase for hex (size_t)
 * @param base  The base you want to use             (size_t)
 *
 * @return int >=0 | -1
 */
ssize_t f_to_hex(unsigned long int size, size_t upper, size_t base, int fd);

/**
 * @brief Print a interger to stdout
 *
 * @param size  Interger to be printed               (int)
 * @param upper Is it uppercase or lowercase for hex (size_t)
 * @param base  The base you want to use             (size_t)
 *
 * @return int >=0 | -1
 */
ssize_t f_format_int(int size, size_t upper, size_t base, int fd);

/**
 * @brief Print addres of pointer to stdout
 *
 * @param ptr   Pointer that wil be printed          (void*)
 * @param upper Is it uppercase or lowercase for hex (size_t)
 * @param base  The base you want to use             (size_t)
 *
 * @return int >=0 | -1
 */
ssize_t f_format_ptr(void *ptr, size_t upper, size_t base, int fd);

/**
 * @brief A simple print function that takes arguments
 *
 * @param s   String that must be printed          (char*)
 * @param ... Arguments to the print in the string
 *
 * @return int >=0 | -1
 */
int ft_fprintf(int fd, const char *s, ...);

/**
 * @brief Write a string to stdout
 *
 * @param s String to print (char*)
 *
 * @return int >=0 | -1
 */
ssize_t f_write_str(const char *s, int fd);

ssize_t f_write_ascii(int c, int fd);
#endif
