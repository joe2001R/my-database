#include "utilities.h"
#include "parser.h"
#include "table.h"

#include <stdio.h>

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        return 1;
    }

    table* m_table = table_db_open(argv[1]);

    while(1)
    {
        string_buffer buffer;
        print_prompt();
        string_buffer_read(&buffer);
        
        if(is_meta_command(&buffer))
        {
            do_meta_command(&buffer,m_table);
        }
        else
        {
            statement m_statement;
            PrepareResult m_prepare_result = prepare_statement(&buffer, &m_statement);

            if(m_prepare_result != PREPARE_SUCCESS)
            {
                fprintf(stdout,"please try again : could not prepare statement ( %s )\n", PREPARE_RESULT_STRING[m_prepare_result]);
                continue;
            }
            
            ENSURE(execute_statement(&m_statement, m_table) == EXECUTE_SUCCESS, "could not execute statement");
        }

        string_buffer_destroy(&buffer);
    }

    return 0;
}