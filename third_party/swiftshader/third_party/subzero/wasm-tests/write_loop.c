// This is derived from the loop in musl's __fwritex that looks for newlines.

int puts(const char *s);

int main(int argc, const char **argv) {
  const char *p = (const char *)argv;
  char *s = "Hello\nWorld";
  unsigned i = 0;
  // Depend on argc to avoid having this whole thing get dead-code-eliminated.
  for (i = 14 - argc; i && p[i - 1] != '\n'; i--)
    ;
  puts(s);
  return i;
}
