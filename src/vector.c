#include "utilities.h"
#include "vector.h"

#include <stdlib.h>
#include <string.h>

void vector_init(void **array, uint32_t *capacity, uint32_t *size,uint32_t element_size)
{
    *array = Malloc(element_size);
    *capacity = 1;
    *size = 0;
}

void vector_push_back(void **array, uint32_t *capacity, uint32_t *size, void *element, uint32_t element_size)
{
    if (*capacity == *size)
    {
        void *new_array = Malloc(*capacity * element_size * 2);
        memcpy(new_array, *array, *capacity * element_size);
        free(*array);

        *array = new_array;
        *capacity = *capacity * 2;
    }

    memcpy(*array + *size * element_size, element, element_size);
    *size = *size + 1;
}

void vector_read(const void **array, uint32_t *size, uint32_t at, void *store_element, uint32_t element_size)
{
    ENSURE(at < *size, "error - out of bounds read : size = %d , at = %d", *size, at);

    memcpy(store_element, *array + at * element_size, element_size);
}