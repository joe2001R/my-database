#include "utilities.h"
#include "parser.h"
#include "table.h"

#include <stdio.h>

static PrepareResult prepare_statement_elog(string_buffer* buffer,statement* statement)
{
    PrepareResult prepare_result = prepare_statement(buffer,statement);

    if (prepare_result != PREPARE_SUCCESS)
    {
        fprintf(stdout, "please try again : could not prepare statement ( %s )\n", PREPARE_RESULT_STRING[prepare_result]);
    }

    return prepare_result;
}

static void execute_statement_elog(statement* statement,table* table)
{
    ExecuteResult execute_result = execute_statement(statement,table);

    if(execute_result != EXECUTE_SUCCESS)
    {
        fprintf(stdout,"Error: could not execute statement ( %s )\n",EXECUTE_RESULT_STRING[execute_result]);
        if(statement->statement_error != NULL)
        {
            fprintf(stdout,"Problem with given input: %s\n",statement->statement_error);
        }
    }
}

int main(int argc, char** argv)
{
    ENSURE(argc>=2,"Error: missing db filename input argument");

    table* table = table_db_open(argv[1]);

    while(1)
    {
        string_buffer buffer;
        print_prompt();
        string_buffer_read(&buffer);
        
        if(is_meta_command(&buffer))
        {
            do_meta_command(&buffer,table);
        }
        else
        {
            statement* statement = create_statement();
            PrepareResult prepare_result = prepare_statement_elog(&buffer,statement);
            
            if(prepare_result == PREPARE_SUCCESS)
            {
                execute_statement_elog(statement, table);
            }

            destroy_statement(statement);
        }

        string_buffer_destroy(&buffer);
    }

    return 0;
}
