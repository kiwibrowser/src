#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

int
main (int argc, char **argv)
{
        const char *str1 = "World ";
        const int expected_pos1[]={0,1,2,3,4,3};

        return check_cursor_pos(str1, expected_pos1);        
}
