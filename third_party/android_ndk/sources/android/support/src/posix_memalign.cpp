#include <errno.h>
#include <malloc.h>
#include <stdlib.h>

int posix_memalign(void** memptr, size_t alignment, size_t size) {
  if ((alignment & (alignment - 1)) != 0 || alignment == 0) {
    return EINVAL;
  }

  if (alignment % sizeof(void*) != 0) {
    return EINVAL;
  }

  *memptr = memalign(alignment, size);
  if (*memptr == NULL) {
    return errno;
  }

  return 0;
}
