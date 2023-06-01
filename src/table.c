#include "btree.h"
#include "table.h"
#include "pager.h"

#include <stdlib.h>
#include<stdio.h>
#include <unistd.h>

/*** private ***/

static struct _LeafNodeRowPair
{
    void* node;
    void* row;
};

typedef struct _LeafNodeRowPair LeafNodeRowPair;

static LeafNodeRowPair find_row(void *node, uint32_t key, uint32_t *row_index, pager *pager)
{
    if(*node_get_type(node)==LEAF_NODE)
    {
        LeafNodeRowPair leaf_node_row_pair = {node, leaf_node_find_row(node, key, row_index)};
        return leaf_node_row_pair;
    }

    else if(*node_get_type(node)==INTERNAL_NODE)
    {
        return find_row(internal_node_find_node(node,key,pager),key,row_index,pager);
    }
    
    fprintf(stderr,"Invalid node type in `find_row` call\n");
    exit(EXIT_FAILURE);
}

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
    returned_cursor->m_end_of_table = (*leaf_node_get_num_records(root_node)==0);

    return returned_cursor;
}

void cursor_advance(cursor *cursor)
{
    if(cursor->m_row_index + 1 < *leaf_node_get_num_records(cursor->m_leaf_node))
    {
        cursor->m_row_index++;
        return;
    }

    uint32_t right_child_page_id = *leaf_node_get_right_child(cursor->m_leaf_node);

    if(right_child_page_id == 0)
    {
        cursor->m_end_of_table = true;
        return;
    }

    cursor->m_leaf_node = pager_get_valid_page_ensure(cursor->m_table->pager,*leaf_node_get_right_child(cursor->m_leaf_node));
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

    ensure(leaf_node_row_pair.node!= NULL,"Error: could not find corresponding leaf node in `table_db_insert`\n");

    void* serialized_row = Malloc(ROW_SIZE);
    row_serialize(serialized_row, row_to_insert);

    leaf_node_insert_row(leaf_node_row_pair.node,key,serialized_row,table);

    destroy(&serialized_row);
}