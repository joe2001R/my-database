#ifndef TABLE_H
#define TABLE_H

#include <stddef.h>

#include "pager.h"
#include "row.h"

#define NUM_ENTRIES_PER_PAGE ( PAGE_SIZE / ROW_SIZE ) // temporary

typedef struct _table
{
    pager* pager;
} table;

table* table_db_open(const char* filename);
void table_db_close(table* table);

#endif