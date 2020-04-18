#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "brl_checks.h"

int main(int argc, char **argv)
{
  int ret = 0;
  char *tables = "da-dk-g26.ctb";
  char *word = "alderen";
  char * hyphens = calloc(9, sizeof(char));

  hyphens[0] = '0';
  hyphens[1] = '0';
  hyphens[2] = '1';
  hyphens[3] = '0';
  hyphens[4] = '1';
  hyphens[5] = '0';
  hyphens[6] = '0';

  ret = check_hyphenation(tables, word, hyphens);
  assert(hyphens[7] == '\0');
  assert(hyphens[8] == '\0');
  free(hyphens);
  return ret;

}
