#ifndef TABLE_DEF_H
#define TABLE_DEF_H

#include "pager.fwd.h"
#include "table.fwd.h"

#include <stdint.h>

typedef struct _table
{
    pager* pager;
    uint32_t root_page_index;

} table;

typedef struct _cursor
{
    table *m_table;
    void *m_leaf_node;
    uint32_t m_row_index;
    bool m_end_of_table;
} cursor;

#endif