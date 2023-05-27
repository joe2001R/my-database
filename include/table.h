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

typedef struct _cursor
{
    table* m_table;
    void* m_leaf_node;
    uint32_t m_row_index;
} cursor;

table* table_db_open(const char* filename);
void table_db_close(table* table);

void table_init_root(table* table);

cursor* table_db_find(table* table,uint32_t id);

void* cursor_read(cursor* cursor);

#endif