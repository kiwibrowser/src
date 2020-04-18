#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

int
main(int argc, char **argv)
{

  int result = 0;

  const char* txt = "Fussball-Vereinigung";

  const char* table = "inpos_match_replace.ctb";

  const char* brl = "FUSSBALL-V7EINIGUNG";

  const int inpos[] = {0,1,2,3,4,5,6,7,8,9,9,12,13,14,15,16,17,18,19};

  result |= check_translation(table, txt, NULL, brl);
  result |= check_inpos(table, txt, inpos);

  return result;

}
