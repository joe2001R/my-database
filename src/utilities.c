#define _GNU_SOURCE

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include<string.h>
#include <sys/param.h>
#include <sys/types.h>

#include "utilities.h"

/*** private functions ***/

static void vector_init(void **array, uint32_t *capacity, uint32_t *size)
{
    *array = Malloc(sizeof(uint32_t));
    *capacity = 1;
    *size = 0;
}

static void vector_push_back(void **array, uint32_t *capacity, uint32_t *size, void *element, uint32_t element_size)
{
    if (*capacity == *size)
    {
        void *new_array = Malloc(*capacity * element_size * 2);
        memcpy(new_array, *array, *capacity * element_size);
        free(*array);

        *array = new_array;
    }

    memcpy(*array + *size * element_size, element, element_size);
    *size = *size + 1;
}

static void vector_read(const void **array, uint32_t *size, uint32_t at, void *store_element, uint32_t element_size)
{
    ENSURE(at < *size, "error - out of bounds read : size = %d , at = %d", *size, at);

    memcpy(store_element, *array + at * element_size, element_size);
}

/***********************************************************/

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
    free(buffer->string);
    buffer->string = NULL;
}

void string_buffer_read(string_buffer *buffer)
{
    ssize_t bytes_read = getline(&buffer->string, &buffer->buffer_capacity, stdin);
    ENSURE(bytes_read > 0, "unsuccessful `getline`");

    buffer->buffer_size = bytes_read - 1;
    buffer->string[bytes_read - 1] = '\0';
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
    
    if(buffer->buffer_size == 0)
    {
        buffer->string[0]='\0';
    }

    ENSURE(buffer->string != NULL, "Error: not enough memory to realloc");

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

void id_vector_init(id_vector *this)
{
    vector_init((void**)&this->array,&this->capacity,&this->size);
}

void id_vector_push_back(id_vector *this, uint32_t id)
{
    vector_push_back((void **)&this->array, &this->capacity, &this->size, (void*)&id, sizeof(uint32_t));
}

uint32_t id_vector_read(id_vector *this, uint32_t at)
{
    uint32_t read_value;

    vector_read((const void**)&this->array,&this->size,at,&read_value,sizeof(uint32_t));

    return read_value;
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