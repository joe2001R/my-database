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

typedef struct _insert_statement_data
{
    row_vector rows_to_insert;
} insert_statement_data;

typedef struct _update_statement_data
{
    row_vector rows_to_update;
} update_statement_data;

#define UNPACK(type,vptr) ((type*) vptr)

#define UNPACK_SELECT_DATA(vptr) UNPACK(select_statement_data,vptr)
#define UNPACK_INSERT_DATA(vptr) UNPACK(insert_statement_data,vptr)
#define UNPACK_UPDATE_DATA(vptr) UNPACK(update_statement_data,vptr)

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

static bool is_valid_id(const char* id)
{
    errno = 0;
    char *endptr;

    return (strtol(id, &endptr, 10) >= 0)   &&
            endptr != id                    &&
            endptr[0] == '\0';
}

static ExecuteResult execute_update(statement *statement, table *table)
{
    ExecuteResult result = EXECUTE_SUCCESS;

    for(uint32_t i = 0; i < UNPACK_UPDATE_DATA(statement->statement_data)->rows_to_update.size; i++)
    {
        row read_row = row_vector_read(&UNPACK_UPDATE_DATA(statement->statement_data)->rows_to_update,i);

        int error = table_db_update(table,read_row.id,&read_row);

        if(error == UPDATE_EMPTY_DB)
        {
            result = EXECUTE_UPDATE_EMPTY_DB;
            break;
        }
        else if(error == UPDATE_ROW_NOT_PRESENT)
        {
            const char* format = "id: %d, name:%s";
            size_t string_len = snprintf(NULL,0,format,read_row.id,read_row.name);

            char* error_msg = Malloc(string_len + 1);
            sprintf(error_msg,format,read_row.id,read_row.name);

            statement->statement_error = error_msg;

            result=EXECUTE_UPDATE_ROW_NOT_FOUND;
            break;
        }
    }
    row_vector_destroy(&UNPACK_UPDATE_DATA(statement->statement_data)->rows_to_update);

    return result;
}

static PrepareResult prepare_select(string_buffer *buffer, statement *statement)
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

static PrepareResult prepare_insert(string_buffer *buffer, statement *statement)
{
    statement->statement_type = INSERT_STATEMENT;
    
    statement->statement_data = Malloc(sizeof(insert_statement_data));

    row_vector_init(&(UNPACK_INSERT_DATA(statement->statement_data)->rows_to_insert));

    strtok(buffer->string, " "); // keyword

    while(1)
    {
        char* id = strtok(NULL," ");
        char* name = strtok(NULL," ,");

        if(id == NULL || name == NULL)
        {
            break;
        }

        if (!is_valid_id(id))
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

static ExecuteResult execute_insert(statement *statement, table *table)
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

static ExecuteResult execute_select(statement *statement, table *table)
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

static PrepareResult prepare_update(string_buffer *buffer, statement *statement)
{
    statement->statement_type = UPDATE_STATEMENT;

    statement->statement_data = Malloc(sizeof(update_statement_data));

    row_vector_init(&UNPACK_UPDATE_DATA(statement->statement_data)->rows_to_update);

    strtok(buffer->string," ");//key word

    while(1)
    {
        char* id = strtok(NULL," ");
        char* name = strtok(NULL," ,");

        if(id == NULL || name == NULL)
        {
            break;
        }

        if(!is_valid_id(id))
        {
            return PREPARE_UPDATE_INVALID_ID;
        }

        if(strlen(name) > NAME_MAX_LENGTH)
        {
            return PREPARE_UPDATE_STRING_TOO_BIG;
        }

        row read_row;
        read_row.id = atoi(id);
        strcpy(read_row.name,name);

        row_vector_push_back(&UNPACK_UPDATE_DATA(statement->statement_data)->rows_to_update,read_row);
    }

    return PREPARE_SUCCESS;
}

/*************************************************************************/

statement* create_statement()
{
	statement* return_value = Malloc(sizeof(statement));

	return_value->statement_data = NULL;
    return_value->statement_error = NULL;

	return return_value;
}

void destroy_statement(statement* statement)
{
	DESTROY(statement->statement_data);
    DESTROY(statement->statement_error);
	DESTROY(statement);
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
	    fputs(buffer.string,stdout);
        string_buffer_destroy(&buffer);
    }
    else if(strcmp(buffer->string,".btree") == 0)
    {
        string_buffer buffer = table_print_btree(table);
        fputs(buffer.string,stdout);
        string_buffer_destroy(&buffer);
    }
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
    else if(statement->statement_type == UPDATE_STATEMENT)
    {
        return execute_update(statement,table);
    }

    fprintf(stderr,"executing invalid statement");
    exit(EXIT_FAILURE);
}

typedef PrepareResult (*prepare_statement_fn)(string_buffer*,statement*);

PrepareResult prepare_statement(string_buffer *buffer, statement *statement)
{
    char* string_copy = strdup(buffer->string);
    const char* keyword = strtok(string_copy," ");

    prepare_statement_fn callback = NULL;

    if(strcmp(keyword,"insert") == 0)
    {
        callback = prepare_insert;
    }
    else if(strcmp(keyword,"select") == 0)
    {
        callback = prepare_select;
    }
    else if(strcmp(keyword,"update") == 0)
    {
        callback = prepare_update;
    }

    free(string_copy);

    if(callback == NULL)
    {
        return PREPARE_UNRECOGNIZED;
    }

    return callback(buffer,statement);
}
