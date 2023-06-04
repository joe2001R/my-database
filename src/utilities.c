#define _GNU_SOURCE

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include<string.h>
#include <sys/param.h>
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
    ENSURE(Malloc, ptr != NULL, "error: could not allocate %ld bytes in heap", size);

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
    free(buffer->string);
    buffer->string = NULL;
}

void string_buffer_read(string_buffer *buffer)
{
    ssize_t bytes_read = getline(&buffer->string, &buffer->buffer_capacity, stdin);
    ENSURE(string_buffer_read, bytes_read > 0, "unsuccessful `getline`");

    buffer->buffer_size = bytes_read - 1;
    buffer->string[bytes_read - 1] = '\0';
}

void string_buffer_store(string_buffer *buffer, const char *string)
{
    ENSURE(string_buffer_store, string != NULL, "error: storing an empty string into a string_buffer");
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
    
    if(buffer->buffer_size == 0)
    {
        buffer->string[0]='\0';
    }

    ENSURE(string_buffer_append, buffer->string != NULL, "Error: not enough memory to realloc");

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

    ENSURE(string_buffer_append2,size>0,"Error:  unsuccessful vsnprintf call");

    char* temp_str = (char*)Malloc(size + 1);

    va_start(args,format);
    vsnprintf(temp_str,size + 1,format,args);
    va_end(args);

    string_buffer_append(buffer,temp_str);

    DESTROY(temp_str);
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
    ENSURE(id_vector_read, at < this->size, "error - out of bounds read : size = %d , at = %d", this->size, at);

    return this->array[at];
}

void id_vector_destroy(id_vector *this)
{
    DESTROY(this->array);
}

void destroy(void **ptr)
{
    free(*ptr);
    *ptr = NULL;
}