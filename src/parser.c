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

static ExecuteResult execute_select_all(table *table)
{
    cursor* cursor = table_db_begin(table);

    if(cursor == NULL)
    {
        return EXECUTE_SUCCESS;
    }

    while(!cursor_is_end(cursor))
    {
        row row_to_display;

        row_deserialize(&row_to_display,cursor_read(cursor));
        print_row(&row_to_display);

        cursor_advance(cursor);
    }

    destroy((void**)&cursor);

    return EXECUTE_SUCCESS;
}

static ExecuteResult execute_select_subset(statement* statement,table* table)
{
    for (int i = 0; i < statement->selected_ids.size; i++)
    {
        row row_to_display;

        cursor *cursor = table_db_find(table, id_vector_read(&statement->selected_ids, i));

        if (cursor == NULL)
        {
            continue;
        }

        row_deserialize(&row_to_display, cursor_read(cursor));

        destroy((void **)&cursor);

        print_row(&row_to_display);
    }

    id_vector_destroy(&statement->selected_ids);

    return EXECUTE_SUCCESS;
}

/*************************************************************************/

void print_prompt()
{
    printf("db > ");
}

bool is_meta_command(string_buffer* buffer)
{
    return buffer->buffer_size!=0 && buffer->string[0]=='.';
}

void do_meta_command(string_buffer *buffer, table *table)
{
    if (strcmp(buffer->string, ".exit") == 0)
    {
        table_db_close(table);
        exit(EXIT_SUCCESS);
    }
    else if(strcmp(buffer->string,".diagnostic") == 0)
    {
        string_buffer buffer = btree_get_diagnostics();
        printf(buffer.string);
        string_buffer_destroy(&buffer);
    }
}

PrepareResult prepare_statement(string_buffer *buffer, statement *statement)
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

PrepareResult prepare_select(string_buffer *buffer, statement *statement)
{
    statement->statement_type = SELECT_STATEMENT;

    strtok(buffer->string, " "); // keyword
    char *id = strtok(NULL, " ");

    if(id!=NULL && strcmp(id,"*") == 0)
    {
        if (strtok(NULL, " ") != NULL)
        {
            return PREPARE_UNRECOGNIZED;
        }

        statement->select_all = true;

        return PREPARE_SUCCESS;
    }

    id_vector_init(&statement->selected_ids);

    while(id)
    {
        if(atoi(id)<0)
        {
            return PREPARE_SELECT_BAD_ID; 
        }
        id_vector_push_back(&statement->selected_ids,atoi(id));

        id = strtok(NULL," ");
    }

    return PREPARE_SUCCESS;
}

PrepareResult prepare_insert(string_buffer *buffer, statement *statement)
{
    statement->statement_type = INSERT_STATEMENT;
    
    strtok(buffer->string," "); // keyword
    char* id = strtok(NULL," ");
    char* name = strtok(NULL," ");
    
    if(id == NULL || name == NULL || atoi(id)<0) 
    {
        return PREPARE_INSERT_INVALID_ARGS;
    }
    
    if( strlen(name)> NAME_MAX_LENGTH )
    {
        return PREPARE_INSERT_STRING_TOO_BIG;
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

static bool pager_is_empty(pager* this)
{
    for(int i = 0; i < MAX_PAGE_NO;i++)
    {
        if(this->pages[i]!=NULL)
        {
            return false;
        }
    }

    return true;
}

ExecuteResult execute_insert(statement *statement, table *table)
{
    if (table->pager->file_length == 0 && pager_is_empty(table->pager))
    {
        table_init_root(table);
    }

    void* page = pager_get_page(table->pager,table->root_page_index);
    
    void* row_to_insert = Malloc(ROW_SIZE);
    row_serialize(row_to_insert,&statement->row_to_insert);

    leaf_node_insert_row(page,statement->row_to_insert.id,row_to_insert,table);
    
    free(row_to_insert);

    return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(statement *statement, table *table)
{
    if(statement->select_all == false)
    {
        return execute_select_subset(statement,table);
    }
    else
    {
        return execute_select_all(table);
    }
}