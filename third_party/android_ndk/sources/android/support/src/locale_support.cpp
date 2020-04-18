#include <stdlib.h>

long strtol_l(const char *nptr, char **endptr, int base, locale_t loc) {
    return strtol(nptr, endptr, base);
}

unsigned long strtoul_l(const char *nptr, char **endptr, int base, locale_t loc) {
    return strtoul(nptr, endptr, base);
}

double strtod_l(const char* nptr, char** endptr, locale_t __unused locale) {
  return strtod(nptr, endptr);
}

float strtof_l(const char* nptr, char** endptr, locale_t __unused locale) {
  return strtof(nptr, endptr);
}
