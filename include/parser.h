#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#include "utilities.h" 
#include "row.h"
#include "table.h"

typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_FAILURE,
    PREPARE_UNRECOGNIZED,
    PREPARE_INSERT_INVALID_ARGS,
    PREPARE_INSERT_STRING_TOO_BIG,
    PREPARE_SELECT_BAD_ID

} PrepareResult;

typedef enum
{
    EXECUTE_SUCCESS
} ExecuteResult;

typedef enum
{
    SELECT_STATEMENT, 
    INSERT_STATEMENT
} StatementType;

typedef struct _statement
{
    StatementType statement_type;
    row row_to_insert;
    id_vector selected_ids;
    bool select_all;
} statement; 

void print_prompt();

bool is_meta_command(string_buffer* buffer);
void do_meta_command(string_buffer* buffer,table* table);

PrepareResult prepare_statement(string_buffer* buffer, statement* statement);
PrepareResult prepare_select(string_buffer* buffer, statement* statement);
PrepareResult prepare_insert(string_buffer* buffer, statement* statement);

ExecuteResult execute_statement(statement* statement,table* table);
ExecuteResult execute_insert(statement *statement, table *table);
ExecuteResult execute_select(statement *statement, table *table);

#endif 