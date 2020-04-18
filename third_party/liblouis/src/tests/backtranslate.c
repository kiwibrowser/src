#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"
#include "brl_checks.h"

typedef struct test {
  char *input;
  char *expected;
} test_s;

test_s tests[] = {
  {",k5", "Ken"},
  {",m5", "Men"},
  {",m>k", "Mark"},
  {",f5", "Fen"},
  {",l5", "Len"},
  {",t5", "Ten"},
  NULL
};

int main(int argc, char **argv) {
  int result = 0;
  const char *tbl = "en-us-g2.ctb";

  for (int i = 0; tests[i].input; i++)
    result |= check_backtranslation(tbl, tests[i].input, NULL, tests[i].expected);

  return result;
}
