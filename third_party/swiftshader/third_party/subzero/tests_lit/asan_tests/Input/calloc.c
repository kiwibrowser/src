#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(void) {
  void *buf = calloc(14, sizeof(int));
  strcpy(buf, "Hello, world!");
  printf("%s\n", buf);
  free(buf);
}
