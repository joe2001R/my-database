#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utilities.h"
#include "pager.h"
#include "row.h"
#include "table.fwd.h"

table* table_db_open(const char* filename);
void table_db_close(table* table);

bool table_db_is_empty(table* table);

string_buffer table_print_btree(table* table);

void table_init_root(table* table);
void table_find_root(table* table);

cursor* table_db_begin(table* table);
cursor* table_db_find(table* table,uint32_t id);
void    table_db_insert(table* table,uint32_t key,row* row_to_insert);

void* cursor_read(cursor* cursor);

void cursor_advance(cursor* cursor);
bool cursor_is_end(cursor* cursor); 

#endif