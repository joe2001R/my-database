#define _GNU_SOURCE

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "utilities.h"

void ensure(bool condition, const char *error_message, ...)
{
    va_list args;
    va_start(args, error_message);
    if (condition == false)
    {
        vfprintf(stderr, error_message, args);
        va_end(args);
        exit(EXIT_FAILURE);
    }
    va_end(args);
}

void input_buffer_init(input_buffer *buffer)
{
    buffer->buffer_size = 0;
    buffer->string = NULL;
}

void input_buffer_destroy(input_buffer *buffer)
{
    buffer->buffer_size = 0;
    free(buffer->string);
    buffer->string = NULL;
}

void read_input(input_buffer *buffer)
{
    ssize_t bytes_read = getline(&buffer->string, &buffer->buffer_size, stdin);
    ensure(bytes_read > 0, "unsuccessful `getline` in `read_input`\n");

    buffer->buffer_size = bytes_read - 1;
    buffer->string[bytes_read - 1] = '\0';
}
