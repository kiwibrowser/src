#include <stdio.h>
#include "brl_checks.h"
#include "liblouis.h"

int
main (int argc, char **argv)
{
  int result = 0;
  char *table = "empty.ctb";
  char rule[18];

  lou_compileString(table, "include latinLetterDef6Dots.uti");

  for (char c1 = 'a'; c1 <= 'z'; c1++) {
    for (char c2 = 'a'; c2 <= 'z'; c2++) {
      for (char c3 = 'a'; c3 <= 'z'; c3++) {
	sprintf(rule, "always aa%c%c%c 1", c1, c2, c3);
	lou_compileString(table, rule);
      }
    }
  }

  result |= check_translation(table, "aaaaa", NULL, "a");
  /* Strangely enough subsequent translations fail */
  result |= check_translation(table, "aaaaa", NULL, "a");
  result |= check_translation(table, "aazzz", NULL, "a");

  return result;
}
