#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

#undef TRANSLATION_TABLE
#define TRANSLATION_TABLE "en-gb-g1.utb"

int
main(int argc, char **argv)
{

  int result = 0;

  const char *str      = "This is a Test";
  const char *typeform = "00000000000000";
  const char *expected = ",this is a ,test";

  result |= check_translation(TRANSLATION_TABLE, str, convert_typeform(typeform), expected);

  str      = "This is a Test in Italic.";
  typeform = "1111111111111111111111111";
  expected = "..,this is a ,test in ,italic4.'";

  result |= check_translation(TRANSLATION_TABLE, str, convert_typeform(typeform), expected);

  str      = "This is a Test";
  typeform = "00000111100000";
  expected = ",this .is .a ,test";

  result |= check_translation(TRANSLATION_TABLE, str, convert_typeform(typeform), expected);

  /* Test case requested here:
   * http://www.freelists.org/post/liblouis-liblouisxml/Mesar-here-are-some-test-possibilities */

  str      = "time and spirit";
  typeform = "111111111111111";
  expected = ".\"t .& ._s";

  result |= check_translation(TRANSLATION_TABLE, str, convert_typeform(typeform), expected);
  return result;
}
