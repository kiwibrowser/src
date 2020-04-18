#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, const char **argv) {
  char *str = "Hello, World!\n";
  const int len = strlen(str);
  write(1, str, len);
  return 0;
}
