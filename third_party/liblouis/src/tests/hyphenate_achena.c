#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "brl_checks.h"

int main(int argc, char **argv)
{
  int ret = 0;
  char *tables = "da-dk-g26.ctb";
  char *word = "achena";
  char * hyphens = calloc(8, sizeof(char));

  hyphens[0] = '0';
  hyphens[1] = '1';
  hyphens[2] = '0';
  hyphens[3] = '0';
  hyphens[4] = '1';
  hyphens[5] = '0';

  ret = check_hyphenation(tables, word, hyphens);
  assert(hyphens[6] == '\0');
  assert(hyphens[7] == '\0');
  free(hyphens);
  return ret;
}
