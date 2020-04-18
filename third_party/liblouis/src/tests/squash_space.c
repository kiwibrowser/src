#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "brl_checks.h"

int main(int argc, char **argv)
{

  int i;
  int result = 0;

  const char *tests[] = {
    /* first column is the text, the second is the expected value */
    " ", " ",
    /* the usual case */
    "                                                                                ", " ",
    /* a very long string */
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                ", " ",
    /* an even longer string */
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                ", " ",
    /* a string containing tabs */
    "    \t",     " \t",
    "    \t    ", " \t ",
    "\t    ",     "\t ",
    /* Strings containing newlines */
    "    \n",     " \n",
    "    \n    ", " \n ",
    "\n    ",     "\n "
  };

  /* A number of strings that we want to squash, i.e. they should all
     result in an output of one space */
  const char *strings[] = {
    /* a simple space */
    " ",
    /* a long string */
    "                                                                                ",
    /* a very long string */
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                ",
    /* an even longer string */
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                " \
    "                                                                                ",
    /* a couple of strings containing tabs */
    "    \t",
    "    \t    ",
    "\t    ",
    /* Strings containing newlines */
    "    \n",
    "    \n    ",
    "\n    ",
    /* All mixed */
    "    \n    \t    \n    \t\t    \n\n\n\t\t    ",
  };
  const char *expected = " ";

  int tests_len = sizeof(tests)/sizeof(char*);

  for (i = 0; i < tests_len; i += 2)
    result |= check_translation("squash_space_with_repeated.utb", tests[i], NULL, tests[i+1]);

  tests_len = sizeof(strings)/sizeof(char*);

  for (i = 0; i < tests_len; i++)
    result |= check_translation("squash_space_with_correct.utb", strings[i], NULL, expected);

  for (i = 0; i < tests_len; i++)
    result |= check_translation("squash_space_with_context_1.utb", strings[i], NULL, expected);

  for (i = 0; i < tests_len; i++)
    result |= check_translation("squash_space_with_context_2.utb", strings[i], NULL, expected);


  return result;
}
