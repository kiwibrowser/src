int foo() { return 5; }

int bar() { return 6; }

int baz() { return 7; }

int (*TABLE[])() = {foo, baz, bar, baz};

int main(int argc, const char **argv) {
  int (*f)() = TABLE[argc - 1];

  return f();
}
