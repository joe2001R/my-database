#define _GNU_SOURCE

#include <readline/readline.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>

#include "parser.fwd.h"
#include "utilities.h"

void ensure(bool condition, const char *error_message, ...)
{
    va_list args;
    if (condition == false)
    {
        va_start(args, error_message);
        vfprintf(stderr, error_message, args);
        va_end(args);
        exit(EXIT_FAILURE);
    }
}

void* Malloc(size_t size)
{
    void* ptr = malloc(size);
    ENSURE(ptr != NULL, "error: could not allocate %ld bytes in heap", size);

    return ptr;
}

void string_buffer_init(string_buffer *buffer)
{
    buffer->buffer_size = 0;
    buffer->buffer_capacity = 0;
    buffer->string = NULL;
}

void string_buffer_destroy(string_buffer *buffer)
{
    buffer->buffer_size = 0;
    buffer->buffer_capacity = 0;
    DESTROY(buffer->string);
}

void string_buffer_read(string_buffer *buffer)
{
    char* input = readline(PRINT_PROMPT);
    ENSURE(input!=NULL, "unsuccessful `readline`");
    
    buffer->string = input;
    buffer->buffer_size = strlen(input)+1;
}

void string_buffer_store(string_buffer *buffer, const char *string)
{
    ENSURE(string != NULL, "error: storing an empty string into a string_buffer");
    string_buffer_destroy(buffer);

    buffer->string = strdup(string);
    buffer->buffer_size = strlen(buffer->string);
    buffer->buffer_capacity = buffer->buffer_size + 1;
}

void string_buffer_append(string_buffer *buffer, const char *string)
{
    if(string == NULL)
    {
        return;
    }

    size_t new_buffer_size = buffer->buffer_size + strlen(string);

    buffer->string = realloc(buffer->string,MAX(new_buffer_size+1,buffer->buffer_capacity));

    ENSURE(buffer->string != NULL, "Error: not enough memory to realloc");
    
    if(buffer->buffer_size == 0)
    {
        buffer->string[0]='\0';
    }

    buffer->buffer_size = new_buffer_size;
    buffer->buffer_capacity = MAX(new_buffer_size + 1, buffer->buffer_capacity);

    strcat(buffer->string,string);
}

void string_buffer_append2(string_buffer *buffer, const char *format, ...)
{
    va_list args;

    va_start(args,format);
    ssize_t size = vsnprintf(NULL,0,format,args);
    va_end(args);

    ENSURE(size>0,"Error:  unsuccessful vsnprintf call");

    char* temp_str = (char*)Malloc(size + 1);

    va_start(args,format);
    vsnprintf(temp_str,size + 1,format,args);
    va_end(args);

    string_buffer_append(buffer,temp_str);

    DESTROY(temp_str);
}

void destroy(void **ptr)
{
    free(*ptr);
    *ptr = NULL;
}