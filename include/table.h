#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "pager.h"
#include "row.h"
#include "table.fwd.h"

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
    bool m_end_of_table;
} cursor;

table* table_db_open(const char* filename);
void table_db_close(table* table);

void table_init_root(table* table);

cursor* table_db_begin(table* table);
cursor* table_db_find(table* table,uint32_t id);

void* cursor_read(cursor* cursor);

void cursor_advance(cursor* cursor);
bool cursor_is_end(cursor* cursor);

#endif