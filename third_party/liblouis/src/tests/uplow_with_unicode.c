#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

int
main(int argc, char **argv)
{

  int result = 0;

  const char *str1 = "gross";
  const char *str2 = "gro\\x00df";

  const char *expected = "g^";

  result |= check_translation("uplow_with_unicode.ctb", str1, NULL, expected);
  result |= check_translation("uplow_with_unicode.ctb", str2, NULL, expected);

  result |= check_translation("lowercase_with_unicode.ctb", str1, NULL, expected);
  result |= check_translation("lowercase_with_unicode.ctb", str2, NULL, expected);

  return result;
}
