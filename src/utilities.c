#define _GNU_SOURCE

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include<string.h>
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

void* Malloc(size_t size)
{
    void* ptr = malloc(size);
    ensure(ptr!=NULL,"error: could not allocate %ld bytes in heap!\n",size);

    return ptr;
}

void string_buffer_init(string_buffer *buffer)
{
    buffer->buffer_size = 0;
    buffer->string = NULL;
}

void string_buffer_destroy(string_buffer *buffer)
{
    buffer->buffer_size = 0;
    free(buffer->string);
    buffer->string = NULL;
}

void string_buffer_read(string_buffer *buffer)
{
    ssize_t bytes_read = getline(&buffer->string, &buffer->buffer_size, stdin);
    ensure(bytes_read > 0, "unsuccessful `getline` in `string_buffer_read`\n");

    buffer->buffer_size = bytes_read - 1;
    buffer->string[bytes_read - 1] = '\0';
}

void string_buffer_store(string_buffer *buffer, const char *string)
{
    ensure(string!=NULL,"error: storing an empty string into the string_buffer\n");
    string_buffer_destroy(buffer);

    buffer->string = strdup(string);
    buffer->buffer_size = strlen(buffer->string);
}

void id_vector_init(id_vector *this)
{
    this->array = Malloc(sizeof(uint32_t));
    this->capacity = 1;
    this->size = 0;
}

void id_vector_push_back(id_vector *this, uint32_t id)
{
    if(this->capacity == this->size)
    {
        uint32_t* new_array = Malloc(this->capacity * sizeof(uint32_t) * 2);
        memcpy(new_array,this->array,this->capacity * sizeof(uint32_t));
        free(this->array);
        
        this->array = new_array;
    }

    *(this->array + this->size)=id;
    this->size++;
}

uint32_t id_vector_read(id_vector *this, uint32_t at)
{
    ensure(at < this->size,"error: invalid id_vector_read : size = %d , at = %d\n",this->size,at);

    return this->array[at];
}

void id_vector_destroy(id_vector *this)
{
    free(this->array);
}
