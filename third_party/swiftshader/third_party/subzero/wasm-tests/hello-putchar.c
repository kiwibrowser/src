#include <stdio.h>
#include <string.h>
#include <unistd.h>

void write_int_(int fd, int n) {
  if (n > 0) {
    write_int_(fd, n / 10);

    int rem = n % 10;
    char c = '0' + rem;
    write(fd, &c, 1);
  }
}

void write_int(int fd, int n) {
  if (n == 0) {
    write(fd, "0", 1);
  } else {
    if (n < 0) {
      write(fd, "-", 1);
      write_int_(fd, -n);
    } else {
      write_int_(fd, n);
    }
  }
}

void stderr_int(int n) {
  write_int(2, n);
  write(2, "\n", 1);
}

int main(int argc, const char **argv) {
  char *str = "Hello, World!\n";
  for (int i = 0; str[i]; ++i) {
    putchar(str[i]);
  }
  return 0;
}
