#ifndef VECTOR_H
#define VECTOR_H

#include "utilities.h"

#include <stdint.h>

void vector_init(void **array, uint32_t *capacity, uint32_t *size, uint32_t element_size);
void vector_push_back(void **array, uint32_t *capacity, uint32_t *size, void *element, uint32_t element_size);
void vector_read(const void **array, uint32_t *size, uint32_t at, void *store_element, uint32_t element_size);

#define VECTOR_DEF(prefix,underlying)\
    typedef struct _##prefix##_vector\
    {\
        underlying* array;\
        \
        uint32_t size;\
        uint32_t capacity;\
    } prefix##_vector;\

#define VECTOR_OP_DEFINE(prefix,underlying)\
static void prefix##_vector_init(prefix##_vector* this)\
{\
    vector_init((void**)&this->array,&this->capacity,&this->size,sizeof(underlying));\
}\
\
static void prefix##_vector_push_back(prefix##_vector* this,underlying element)\
{\
    vector_push_back((void**)&this->array,&this->capacity,&this->size,(void*)&element,sizeof(underlying));\
}\
\
static underlying prefix##_vector_read(prefix##_vector *this, uint32_t at)\
{\
    underlying read_value;\
    vector_read((const void**)&this->array,&this->size,at,&read_value,sizeof(underlying));\
    return read_value;\
}\
\
static void prefix##_vector_destroy(prefix##_vector* this)\
{\
    DESTROY(this->array);\
}\

#define VECTOR_CLASS(prefix,underlying)\
VECTOR_DEF(prefix,underlying)\
VECTOR_OP_DEFINE(prefix,underlying)\

#endif