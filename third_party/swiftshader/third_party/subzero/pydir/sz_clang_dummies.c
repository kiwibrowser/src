#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void *__asan_dummy_calloc(size_t nmemb, size_t size) {
  return calloc(nmemb, size);
}

#ifdef __cplusplus
} // extern "C"
#endif
