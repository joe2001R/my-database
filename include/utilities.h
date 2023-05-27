#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIZE_OF(STRUCT,MEMBER) sizeof(((STRUCT*)0)->MEMBER) 

void ensure(bool condition,const char* error_message, ...);
void* Malloc(size_t size);
void destroy(void** ptr);

typedef struct
{
    size_t buffer_size;
    char* string;
} string_buffer;

void string_buffer_init(string_buffer* buffer);
void string_buffer_destroy(string_buffer* buffer);
void string_buffer_read(string_buffer *buffer);
void string_buffer_store(string_buffer *buffer, const char *string);

typedef struct _id_vector
{
    uint32_t* array;

    uint32_t size;
    uint32_t capacity;
} id_vector;

void        id_vector_init(id_vector* this);
void        id_vector_push_back(id_vector *this, uint32_t id);
uint32_t    id_vector_read(id_vector* this,uint32_t at);
void        id_vector_destroy(id_vector* this);

#endif