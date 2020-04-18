#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"

int
main(int argc, char **argv)
{
  const char* goodTable = "en-us-g1.ctb";
  const char* badTable = "bad.ctb";
  void* table = NULL;
  int result = 0;

  table = lou_getTable(goodTable);
  if (!table)
    {
      printf("Getting %s failed, expected success\n", goodTable);
      result = 1;
    }
  else
    table = NULL;

  table = lou_getTable(badTable);
  if (table)
    {
      printf("Getting %s succeeded, expected failure\n", badTable);
      table = NULL;
      result = 1;
    }

  table = lou_getTable(goodTable);
  if (!table)
    {
      printf("Getting %s failed, expected success\n", goodTable);
      result = 1;
    }
  else
    table = NULL;

  lou_free();

  return result;
}
