/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <dlfcn.h>
#include <time.h>

typedef time_t (*time_func_t)(time_t*);

/*
 * This union is required only to convert void* to function pointer. ISO C does
 * not provide such a conversion.
 */
typedef union {
  time_func_t pfunc;
  void* pvoid;
} dl_union;

int main(void) {
  void *handle;
  dl_union f;

  handle = dlopen("libstdc++.so.6", RTLD_LAZY);
  assert(NULL != handle);
  dlerror();
  f.pvoid = dlsym(handle, "__atomic_flag_for_address");
  assert(NULL == dlerror());
  assert(-1 != (*f.pfunc)(NULL));
  assert(0 == dlclose(handle));
  return 0;
}
