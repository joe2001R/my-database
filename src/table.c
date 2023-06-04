#include "btree.h"
#include "table.def.h"
#include "table.h"
#include "pager.def.h"
#include "pager.h"

#include <stdlib.h>
#include<stdio.h>
#include <unistd.h>

/*** private ***/

#define ROOT_NODE_PAGE_INDEX 0

/***********************************************/

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

    ENSURE(table_db_close, close(table->pager->fd) == 0, "error closing the file");

    free(table->pager);
    free(table);
}

bool table_db_is_empty(table *table)
{
    return table->pager->file_length == 0 && pager_is_empty(table->pager);
}

string_buffer table_print_btree(table *table)
{
    return btree_print_tree(pager_get_valid_page_ensure(table->pager, table->root_page_index), table->pager);
}

void table_find_root(table *table)
{
    table->root_page_index = ROOT_NODE_PAGE_INDEX;

    ENSURE(table_find_root, node_is_root(pager_get_valid_page_ensure(table->pager, ROOT_NODE_PAGE_INDEX)), "Error: node in page index %d is not root node", ROOT_NODE_PAGE_INDEX);
}

void table_init_root(table *table)
{
    table->root_page_index = ROOT_NODE_PAGE_INDEX;

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
    LeafNodeRowPair leaf_node_row_pair = find_row(root_node, id, &row_index,table->pager);

    if (leaf_node_row_pair.row == NULL)
    {
        return NULL;
    }

    cursor* returned_cursor = Malloc(sizeof(cursor));
    returned_cursor->m_table=table;
    returned_cursor->m_leaf_node = leaf_node_row_pair.node;
    returned_cursor->m_row_index = row_index;
    returned_cursor->m_end_of_table = false;

    return returned_cursor; 
}

const void* cursor_read(cursor *cursor)
{
    return leaf_node_read_row(cursor->m_leaf_node,cursor->m_row_index);
}

cursor* table_db_begin(table *table)
{
    void *root_node = pager_get_valid_page(table->pager, table->root_page_index);
    if (root_node == NULL)
    {
        return NULL;
    }

    cursor* returned_cursor = Malloc(sizeof(cursor));

    LeafNodeRowPair leaf_node_row_pair = find_row(root_node,0,NULL,table->pager);
    
    returned_cursor->m_table = table;
    returned_cursor->m_leaf_node = leaf_node_row_pair.node;
    returned_cursor->m_row_index=0;
    returned_cursor->m_end_of_table = (leaf_node_read_num_records(leaf_node_row_pair.node) == 0);

    return returned_cursor;
}

void cursor_advance(cursor *cursor)
{
    if (cursor->m_row_index + 1 < leaf_node_read_num_records(cursor->m_leaf_node))
    {
        cursor->m_row_index++;
        return;
    }

    if(leaf_node_read_right_child(cursor->m_leaf_node) == 0)
    {
        cursor->m_end_of_table = true;
        return;
    }

    cursor->m_leaf_node = pager_get_valid_page_ensure(cursor->m_table->pager,leaf_node_read_right_child(cursor->m_leaf_node));
    cursor->m_row_index = 0;
}

bool cursor_is_end(cursor *cursor)
{
    return cursor->m_end_of_table;
}

void table_db_insert(table *table, uint32_t key, row *row_to_insert)
{
    void* root_node = pager_get_valid_page_ensure(table->pager,table->root_page_index);
    
    LeafNodeRowPair leaf_node_row_pair = find_row(root_node,key,NULL,table->pager);

    ENSURE(table_db_insert, leaf_node_row_pair.node != NULL, "Error: could not find corresponding leaf node");

    void* serialized_row = Malloc(ROW_SIZE);
    row_serialize(serialized_row, row_to_insert);

    leaf_node_insert_row(leaf_node_row_pair.node,key,serialized_row,table);

    DESTROY(serialized_row);
}