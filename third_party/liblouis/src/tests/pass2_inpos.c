#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

int
main(int argc, char **argv)
{

  const char* table = "pass2.ctb";

  int result = 0;

  /* First check a plain word to see if inpos handling generally
     works */
  const char* str1 = "Rene";
  const int expected_inpos1[] = {0,1,2,3,3};

  result |= check_inpos(table, str1, expected_inpos1);

  /* then try a word which uses pass2 and makes the output longer */
  const char* str2 = "Reno";
  const int expected_inpos2[] = {0,1,2,3,3};

  result |= check_inpos(table, str2, expected_inpos2);

  /* finally try a word also uses pass2, deletes a char from the
     output and essentially shortens the output */
  const char* str3 = "xRen";
  const int expected_inpos3[] = {1,2,3};

  result |= check_inpos(table, str3, expected_inpos3);

  return result;

}
