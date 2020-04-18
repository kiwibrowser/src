#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

int
main (int argc, char **argv)
{
        const char *str1 = "This is a test ";
        const int expected_pos1[]={0,1,2,3,2,3,4,5,6,7,8,9,10,11,11};

        return check_cursor_pos(str1, expected_pos1);        
}
