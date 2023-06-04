#ifndef ROW_H
#define ROW_H

#define NAME_MAX_LENGTH 12

#include <assert.h>
#include <stdint.h>

#include "utilities.h"
 
typedef struct _row
{
    uint32_t id;
    char name[NAME_MAX_LENGTH + 1];
} row;
  
void row_serialize(void *destination,const row *source);
void row_deserialize(row *destination, const void *source);

#define ID_OFFSET (0)
#define ID_SIZE (SIZE_OF(row,id))
#define NAME_OFFSET (ID_SIZE + ID_OFFSET)
#define NAME_SIZE (SIZE_OF(row,name))
#define ROW_SIZE (ID_SIZE + NAME_SIZE)

static_assert(NAME_SIZE == (sizeof(char) * (NAME_MAX_LENGTH + 1)), "invalid row->name size");
static_assert(ID_SIZE == sizeof(uint32_t), "invalid row->id size");

#endif