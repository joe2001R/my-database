#ifndef TABLE_H
#define TABLE_H

#include <stddef.h>
#include <stdint.h>

#include "pager.h"
#include "row.h"

typedef struct _table
{
    pager* pager; 
    uint32_t root_page_index;

} table;

table* table_db_open(const char* filename);
void table_db_close(table* table);

void table_init_root(table* table);

#endif