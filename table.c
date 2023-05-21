#include "table.h"
#include "pager.h"

#include <stdlib.h>
#include <unistd.h>

table *table_db_open(const char *filename)
{
    pager* new_pager = pager_open(filename);

    table* return_value = malloc(sizeof(table));
    return_value->pager = new_pager;

    return return_value;
}

void table_db_close(table* table)
{
    for(int i = 0; i < table->pager->num_pages;i++)
    {
        pager_flush(table->pager,i);
        free(table->pager->pages[i]);
    }
    
    ensure(close(table->pager->fd)==0,"error closing the file\n");

    free(table->pager);
    free(table);
}