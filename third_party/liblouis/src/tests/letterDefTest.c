#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

int main(int argc, char **argv)
{

  int result = 0;

  const char *text = "\\x280d\\x280e";  // "⠍⠎"
  const char *expected = "sm";  // "⠎⠍"

  result |= check_translation("letterDefTest_letter.ctb", text, NULL, expected);
  result |= check_translation("letterDefTest_lowercase.ctb", text, NULL, expected);
  result |= check_translation("letterDefTest_uplow.ctb", text, NULL, expected);
  result |= check_translation("letterDefTest_uppercase.ctb", text, NULL, expected);

  return result;
}
