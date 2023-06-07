#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "btree.h"
#include "parser.h"
#include "row.h"
#include "table.h"

//private functions

VECTOR_OP_DEFINE(id, uint32_t)
VECTOR_OP_DEFINE(row,row)

typedef struct _select_statement_data
{
    bool select_all;
    id_vector selected_ids;
} select_statement_data;

typedef struct _insert_statement_Data
{
    row_vector rows_to_insert;
} insert_statement_data;

#define UNPACK(type,vptr) ((type*) vptr)

#define UNPACK_SELECT_DATA(vptr) UNPACK(select_statement_data,vptr)
#define UNPACK_INSERT_DATA(vptr) UNPACK(insert_statement_data,vptr)

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

    DESTROY(cursor);

    return EXECUTE_SUCCESS;
}

static ExecuteResult execute_select_subset(statement* statement,table* table)
{
    for (int i = 0; i < UNPACK_SELECT_DATA(statement->statement_data)->selected_ids.size; i++)
    {
        row row_to_display;

        cursor *cursor = table_db_find(table, id_vector_read(&UNPACK_SELECT_DATA(statement->statement_data)->selected_ids, i));

        if (cursor == NULL)
        {
            continue;
        }

        row_deserialize(&row_to_display, cursor_read(cursor));

        DESTROY(cursor);

        print_row(&row_to_display);
    }

    id_vector_destroy(&UNPACK_SELECT_DATA(statement->statement_data)->selected_ids);

    return EXECUTE_SUCCESS;
}

/*************************************************************************/

statement* create_statement()
{
	statement* return_value = Malloc(sizeof(statement));

	return_value->statement_data = NULL;

	return return_value;
}

void destroy_statement(statement* statement)
{
	DESTROY(statement->statement_data);
	DESTROY(statement);
}

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
    else if(strcmp(buffer->string,".btree") == 0)
    {
        string_buffer buffer = table_print_btree(table);
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

    statement->statement_data = Malloc(sizeof(select_statement_data));

    if(id!=NULL && strcmp(id,"*") == 0)
    {
        if (strtok(NULL, " ") != NULL)
        {
            return PREPARE_UNRECOGNIZED;
        }

        UNPACK_SELECT_DATA(statement->statement_data)->select_all=true;

        return PREPARE_SUCCESS;
    }

    UNPACK_SELECT_DATA(statement->statement_data)->select_all=false;

    id_vector_init(&UNPACK_SELECT_DATA(statement->statement_data)->selected_ids);

    while(id)
    {
        if(atoi(id)<0)
        {
            return PREPARE_SELECT_BAD_ID; 
        }
        id_vector_push_back(&UNPACK_SELECT_DATA(statement->statement_data)->selected_ids,atoi(id));

        id = strtok(NULL," ");
    }

    return PREPARE_SUCCESS;
}

PrepareResult prepare_insert(string_buffer *buffer, statement *statement)
{
    statement->statement_type = INSERT_STATEMENT;
    
    statement->statement_data = Malloc(sizeof(insert_statement_data));

    row_vector_init(&(UNPACK_INSERT_DATA(statement->statement_data)->rows_to_insert));

    strtok(buffer->string, " "); // keyword

    while(1)
    {
	errno = 0;
        char* id = strtok(NULL," ");
        char* name = strtok(NULL," ,");
	char* endptr;

        if(id == NULL || name == NULL)
        {
            break;
        }
        
        if((strtol(id,&endptr,10)<0) || endptr==id || endptr[0]!='\0') 
        {
            return PREPARE_INSERT_INVALID_ID;
        }
        
        if( strlen(name)> NAME_MAX_LENGTH )
        {
            return PREPARE_INSERT_STRING_TOO_BIG;
        }

        row read_row;
        read_row.id = atoi(id);
        strcpy(read_row.name,name);

        row_vector_push_back(&(UNPACK_INSERT_DATA(statement->statement_data)->rows_to_insert), read_row);
    }

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
    if (table_db_is_empty(table))
    {
        table_init_root(table);
    }
    else
    {
        table_find_root(table);
    }

    for (int i = 0; i < UNPACK_INSERT_DATA(statement->statement_data)->rows_to_insert.size; i++)
    {
        row row_to_insert = row_vector_read(&UNPACK_INSERT_DATA(statement->statement_data)->rows_to_insert,i);

        table_db_insert(table, row_to_insert.id, &row_to_insert);
    }

    row_vector_destroy(&UNPACK_INSERT_DATA(statement->statement_data)->rows_to_insert);

    return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(statement *statement, table *table)
{
    if(UNPACK_SELECT_DATA(statement->statement_data)->select_all == false)
    {
        return execute_select_subset(statement,table);
    }
    else
    {
        return execute_select_all(table);
    }
}
