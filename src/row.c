#include "row.h"

#include <string.h>

void row_serialize(void *destination, row *source)
{
    memcpy(destination, &source->id, ID_SIZE);
    memcpy(destination + NAME_OFFSET, &source->name, NAME_SIZE);
}

void row_deserialize(row *destination, void *source)
{
    memcpy(&destination->id,source,ID_SIZE);
    memcpy(&destination->name,source + NAME_OFFSET,NAME_SIZE );
}
