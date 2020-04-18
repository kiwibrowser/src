#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

int
main (int argc, char **argv)
{
  const char *str1 = "user@example.com";
  const int expected_inpos[] = 
    {0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,15};
  return check_inpos(TRANSLATION_TABLE, str1, expected_inpos);        
}
