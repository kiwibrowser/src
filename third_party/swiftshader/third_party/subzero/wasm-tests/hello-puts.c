#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, const char **argv) {
  fputs("Hello,", stdout);
  fputs(" ", stdout);
  fputs("world\n", stdout);
  return 0;
}
