#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "btree.h"
#include "parser.h"
#include "row.h"
#include "table.h"

//private functions

static void print_row(const row* m_row)
{
    printf("%d %s\n",m_row->id,m_row->name);
}

void print_prompt()
{
    printf("db > ");
}

bool is_meta_command(input_buffer* buffer)
{
    return buffer->buffer_size!=0 && buffer->string[0]=='.';
}

void do_meta_command(input_buffer *buffer, table *table)
{
    if (strcmp(buffer->string, ".exit") == 0)
    {
        table_db_close(table);
        exit(EXIT_SUCCESS);
    }
}

PrepareResult prepare_statement(input_buffer *buffer, statement *statement)
{
    if(strncmp(buffer->string,"insert",6) == 0)
    {
        return prepare_insert(buffer,statement);
    }
    else if(strncmp(buffer->string,"select",6) == 0)
    {
        return prepare_select(buffer,statement);
    }

    return PREPARE_UNRECOGNIZED;
}

PrepareResult prepare_select(input_buffer *buffer, statement *statement)
{
    statement->statement_type = SELECT_STATEMENT;
    return PREPARE_SUCCESS;
}

PrepareResult prepare_insert(input_buffer *buffer, statement *statement)
{
    statement->statement_type = INSERT_STATEMENT;
    
    strtok(buffer->string," "); // keyword
    char* id = strtok(NULL," ");
    char* name = strtok(NULL," ");
    
    if(id == NULL || name == NULL || strlen(name)> NAME_MAX_LENGTH || atoi(id)<0) 
    {
        return PREPARE_FAILURE;
    }

    statement->row_to_insert.id = atoi(id);
    strcpy(statement->row_to_insert.name,name);

    return PREPARE_SUCCESS;
}

ExecuteResult execute_statement(statement *statement, table *table)
{
    if(statement->statement_type == INSERT_STATEMENT)
    {
        return execute_insert(statement,table);
    }
    else if(statement->statement_type == SELECT_STATEMENT)
    {
        return execute_select(statement,table);
    }

    fprintf(stderr,"executing invalid statement");
    exit(EXIT_FAILURE);
}
ExecuteResult execute_insert(statement *statement, table *table)
{
    if(table->pager->file_length == 0)
    {
        table_init_root(table);
    }

    void* page = pager_get_page(table->pager,table->root_page_index);
    
    void* row_to_insert = malloc(ROW_SIZE);
    row_serialize(row_to_insert,&statement->row_to_insert);

    leaf_node_insert_row(page,statement->row_to_insert.id,row_to_insert);

    return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(statement *statement, table *table)
{
    row row_to_display;

    void* page = pager_get_page(table->pager,table->root_page_index);

    row_deserialize(&row_to_display,leaf_node_find_row(page,1));

    print_row(&row_to_display);

    return EXECUTE_SUCCESS;
}