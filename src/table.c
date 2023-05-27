#include "btree.h"
#include "table.h"
#include "pager.h"

#include <stdlib.h>
#include <unistd.h>

table *table_db_open(const char *filename)
{
    pager* new_pager = pager_open(filename);

    table* return_value = Malloc(sizeof(table));
    return_value->pager = new_pager;

    return return_value;
}

void table_db_close(table* table)
{
    for(int i = 0; i < MAX_PAGE_NO;i++)
    {
        if(table->pager->pages[i]==NULL)
        {
            continue;
        }

        pager_flush(table->pager,i);
        free(table->pager->pages[i]);
    }
    
    ensure(close(table->pager->fd)==0,"error closing the file\n");

    free(table->pager);
    free(table);
}

void table_init_root(table *table)
{
    table->root_page_index = 0;

    void* root = pager_get_page(table->pager,table->root_page_index);

    leaf_node_root_init(root);
}

cursor* table_db_find(table *table, uint32_t id)
{
    void* root_node = pager_get_valid_page(table->pager,table->root_page_index);
    if(root_node == NULL)
    {
        return NULL;
    }

    uint32_t row_index;
    void* node = leaf_node_find_row(root_node,id,&row_index);
    
    if(node == NULL)
    {
        return NULL;
    }

    cursor* returned_cursor = Malloc(sizeof(cursor));
    returned_cursor->m_table=table;
    returned_cursor->m_leaf_node = root_node;
    returned_cursor->m_row_index = row_index;

    return returned_cursor; 
}

void* cursor_read(cursor *cursor)
{
    return leaf_node_get_row(cursor->m_leaf_node,cursor->m_row_index);
}

cursor* table_db_begin(table *table)
{
    void *root_node = pager_get_valid_page(table->pager, table->root_page_index);
    if (root_node == NULL)
    {
        return NULL;
    }

    cursor* returned_cursor = Malloc(sizeof(cursor));
    
    returned_cursor->m_table = table;
    returned_cursor->m_leaf_node=root_node;
    returned_cursor->m_row_index=0;

    return returned_cursor;
}

void cursor_advance(cursor *cursor)
{
    cursor->m_row_index++;
}

bool cursor_is_end(cursor *cursor)
{
    return *leaf_node_get_num_records(cursor->m_leaf_node) == cursor->m_row_index;
}