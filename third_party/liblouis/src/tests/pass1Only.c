#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

int
main(int argc, char **argv)
{

  const char* table = "en-us-comp8.ctb";
  int result = 0;

  result = check_translation_with_mode(table, "a", NULL, "a", pass1Only);

  return result;
}
