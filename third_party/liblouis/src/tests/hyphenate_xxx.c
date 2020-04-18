#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "brl_checks.h"

/* Illustrates the same bug as doctests/hyphenate_xxx_test.txt
 * The same bug can be reproduced with only 3 'x'es. */
int main(int argc, char **argv)
{
  int ret = 0;
  char *tables = "cs-g1.ctb,hyph_cs_CZ.dic";
  char *word = "xxx";
  char * hyphens = calloc(5, sizeof(char));

  hyphens[0] = '0';
  hyphens[1] = '0';
  hyphens[2] = '0';

  ret = check_hyphenation(tables, word, hyphens);
  assert(hyphens[3] == '\0');
  assert(hyphens[4] == '\0');
  free(hyphens);
  return ret;
}
