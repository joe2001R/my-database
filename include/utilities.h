#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIZE_OF(STRUCT,MEMBER) sizeof(((STRUCT*)0)->MEMBER) 

void ensure(bool condition,const char* error_message, ...);
void* Malloc(size_t size);
void destroy(void** ptr);

#define DESTROY(var) destroy((void**)&var)
#define ENSURE(condition,err_msg,...) ensure(condition,err_msg " in `%s` call\n",##__VA_ARGS__,__FUNCTION__)

typedef struct
{
    size_t buffer_size;
    size_t buffer_capacity;
    char* string;
} string_buffer;

void string_buffer_init(string_buffer* buffer);
void string_buffer_destroy(string_buffer* buffer);
void string_buffer_read(string_buffer *buffer);
void string_buffer_store(string_buffer *buffer, const char *string);
void string_buffer_append(string_buffer* buffer,const char* string);
void string_buffer_append2(string_buffer* buffer,const char* format,...);

#endif