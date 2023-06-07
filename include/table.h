#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utilities.h"
#include "table.fwd.h"

typedef enum
{
    UPDATE_SUCCESS,
    UPDATE_EMPTY_DB,
    UPDATE_ROW_NOT_PRESENT
};

table* table_db_open(const char* filename);
void table_db_close(table* table);

bool table_db_is_empty(table* table);

string_buffer table_print_btree(table* table);

void table_init_root(table* table);
void table_find_root(table* table);

cursor* table_db_begin(table* table);
cursor* table_db_find(table* table,uint32_t id);
void    table_db_insert(table* table,uint32_t key,row* row_to_insert);
int     table_db_update(table* table,uint32_t key,row* row_to_update);

const void* cursor_read(cursor* cursor);

void cursor_advance(cursor* cursor);
bool cursor_is_end(cursor* cursor); 

#endif