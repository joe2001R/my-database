#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#include "utilities.h" 
#include "row.h"
#include "table.h"
#include "vector.h"

#define FOREACH_PREPARE_ENUM(GENERATOR) \
        GENERATOR(PREPARE_SUCCESS) \
        GENERATOR(PREPARE_FAILURE) \
        GENERATOR(PREPARE_UNRECOGNIZED) \
        GENERATOR(PREPARE_INSERT_INVALID_ID) \
        GENERATOR(PREPARE_INSERT_STRING_TOO_BIG) \
        GENERATOR(PREPARE_SELECT_BAD_ID) \

#define FOREACH_STATEMENT_TYPE_ENUM(GENERATOR) \
        GENERATOR(SELECT_STATEMENT) \
        GENERATOR(INSERT_STATEMENT) \
        GENERATOR(UPDATE_STATEMENT) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

static const char* PREPARE_RESULT_STRING[] = {FOREACH_PREPARE_ENUM(GENERATE_STRING)};

VECTOR_DEF(id, uint32_t)
VECTOR_DEF(row,row)

typedef enum
{
    FOREACH_PREPARE_ENUM(GENERATE_ENUM)
} PrepareResult;

typedef enum
{
    EXECUTE_SUCCESS
} ExecuteResult;

typedef enum
{
    FOREACH_STATEMENT_TYPE_ENUM(GENERATE_ENUM)
} StatementType;

typedef void* StatementData;

typedef struct _statement
{
    StatementType statement_type;
    StatementData statement_data;
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