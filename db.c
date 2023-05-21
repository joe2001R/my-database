#include <stdio.h>
#include "utilities.h"
#include "parser.h"
#include "table.h"

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        return 1;
    }

    table* m_table = table_db_open(argv[1]);

    while(1)
    {
        input_buffer buffer;
        print_prompt();
        read_input(&buffer);
        
        if(is_meta_command(&buffer))
        {
            do_meta_command(&buffer,m_table);
        }
        else
        {
            statement m_statement;

            ensure(prepare_statement(&buffer, &m_statement) == PREPARE_SUCCESS, "could not prepare statement successfully\n");
            ensure(execute_statement(&m_statement, m_table) == EXECUTE_SUCCESS, "could not execute statement\n");
        }

        input_buffer_destroy(&buffer);
    }

    return 0;
}