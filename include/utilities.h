#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdbool.h>
#include <stddef.h>

#define SIZE_OF(STRUCT,MEMBER) sizeof(((STRUCT*)0)->MEMBER) 

void ensure(bool condition,const char* error_message, ...);

typedef struct
{
    size_t buffer_size;
    char* string;
} input_buffer;

void input_buffer_init(input_buffer* buffer);
void input_buffer_destroy(input_buffer* buffer);

void read_input(input_buffer* buffer);

#endif