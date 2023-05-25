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