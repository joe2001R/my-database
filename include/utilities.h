#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIZE_OF(STRUCT,MEMBER) sizeof(((STRUCT*)0)->MEMBER) 

void ensure(bool condition,const char* error_message, ...);

typedef struct
{
    size_t buffer_size;
    char* string;
} input_buffer;

void input_buffer_init(input_buffer* buffer);
void input_buffer_destroy(input_buffer* buffer);

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

void read_input(input_buffer* buffer);

#endif