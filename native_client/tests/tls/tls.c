/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef WITH_PTHREAD
# include <pthread.h>
#endif
#include <stdint.h>
#include <stdio.h>

#include "native_client/src/include/nacl_assert.h"

__thread int tdata1 = 1;
__thread int tdata2 __attribute__((aligned(0x10))) = 2;
/* We need to test the case when TLS size is not aligned to 16 bytes. */
#ifdef MORE_TDATA
__thread int tdata_more = 3;
#endif
#ifdef TDATA_LARGE_ALIGN
__thread int tdata3 __attribute__((aligned(0x1000))) = 4;
#endif
#ifdef WITH_TBSS
__thread int tbss1;
__thread int tbss2 __attribute__((aligned(0x10)));
/* If tdata and tbss are aligned separately, we need to check different tbss
   sizes too. */
# ifdef MORE_TBSS
__thread int tbss_more;
# endif
# ifdef TBSS_LARGE_ALIGN
__thread int tbss3 __attribute__((aligned(0x1000)));
# endif
#endif

#ifdef WITH_PTHREAD
void *thread_proc(void *arg) {
  return arg;
}
#endif

int __attribute__((noinline)) AlignCheck(void *address, int align) {
  if ((uintptr_t) address % align != 0) {
    fprintf(stderr, "Address %p is not aligned to a multiple of %i\n",
            address, align);
    return 1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  int errors = 0;
#ifdef WITH_PTHREAD
  pthread_t tid;
#endif
  if (tdata1 != 1 || tdata2 != 2) {
    errors++;
  }
  errors += AlignCheck(&tdata2, 0x10);
#ifdef MORE_TDATA
  if (tdata_more != 3) {
    errors++;
  }
#endif
#ifdef TDATA_LARGE_ALIGN
  errors += AlignCheck(&tdata3, 0x1000);
#endif

#ifdef WITH_TBSS
  errors += AlignCheck(&tbss2, 0x10);
  ASSERT_EQ(tbss1, 0);
  ASSERT_EQ(tbss2, 0);
  tbss1 = 1;
  tbss2 = 2;
  if (tbss1 != 1 || tbss2 != 2) {
    errors++;
  }
# ifdef MORE_TBSS
  ASSERT_EQ(tbss_more, 0);
# endif
# ifdef TBSS_LARGE_ALIGN
  ASSERT_EQ(tbss3, 0);
  errors += AlignCheck(&tbss3, 0x1000);
# endif
#endif
#ifdef WITH_PTHREAD
  /* This is dead code but it makes linker use pthread library */
  if (argc == -1) {
    pthread_create(&tid, NULL, thread_proc, NULL);
  }
#endif
  return errors;
}
