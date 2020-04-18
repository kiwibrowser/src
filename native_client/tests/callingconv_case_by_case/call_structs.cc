/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Tests if the ABI for passing structures by value matches.
 *
 * We have 4 modules, two compiled by one compiler and two by the other.
 * CC1: MODULE0 and MODULE3, CC2: MODULE1 and MODULE2
 * Overall structure is this:
 * main():
 *   test_type()
 *   vv--call/check--vv
 *   mod0_type()  --call/check-->      mod1_type()
 *                                     vv--call/check--vv
 *   mod3_type()  <--call/check--      mod2_type()
 *
 * So, CC1 passes stuff to CC1 -> CC2 -> CC2 -> CC1
 *
 * In addition, each callee memsets the given struct, and upon return,
 * the caller checks that the original struct was untouched to ensure that the
 * by value passing is done via a copy.
 *
 * To see the pre-processor-generated source for MODULE${X}
 *
 * gcc file.c -E -o - -DMODULE${X} | indent | less
 */

#include "native_client/tests/callingconv_case_by_case/useful_structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Trick inter-procedural constant propagation by adding some branching.
 * This shouldn't happen with LLVM since we have split every function into
 * separate native .o files, but... just in case the build changes.
 * Also tag each function as noinline... just in case.
 */
#if defined(MODULE0)
int should_be_true;
#else
extern int should_be_true;
#endif

/**********************************************************************/
/* The actual test code structure (which is divided into 4 modules). */

#define GENERATE_FOR_MODULE3(TYPE)                                  \
  __attribute__((noinline)) void mod3_##TYPE(TYPE z);               \
  void mod3_##TYPE(TYPE z) {                                        \
    CHECK_##TYPE(z);                                                \
    if (should_be_true) {                                           \
      printf("Made it to mod3_" #TYPE "\n");                        \
      memset((void*)&z, 0, sizeof z);                               \
    } else {                                                        \
      printf("should_be_true is not true for mod3_" #TYPE "\n");    \
      memset((void*)&z, 1, sizeof z);                               \
    }                                                               \
  }

#define GENERATE_FOR_MODULE2(TYPE)                                  \
  extern __attribute__((noinline)) void mod3_##TYPE(TYPE z);        \
  __attribute__((noinline)) void mod2_##TYPE(TYPE z);               \
  void mod2_##TYPE(TYPE z) {                                        \
    CHECK_##TYPE(z);                                                \
    if (should_be_true) {                                           \
      mod3_##TYPE(z);                                               \
      CHECK_##TYPE(z);                                              \
      printf("Made it to mod2_" #TYPE "\n");                        \
      memset((void*)&z, 0, sizeof z);                               \
    } else {                                                        \
      memset((void*)&z, 0, sizeof z);                               \
      printf("should_be_true is not true for mod2_" #TYPE "\n");    \
      mod3_##TYPE(z);                                               \
    }                                                               \
  }

#define GENERATE_FOR_MODULE1(TYPE)                                  \
  extern __attribute__((noinline)) void mod2_##TYPE(TYPE z);        \
  __attribute__((noinline)) void mod1_##TYPE(TYPE z);               \
  void mod1_##TYPE(TYPE z) {                                        \
    CHECK_##TYPE(z);                                                \
    if (should_be_true) {                                           \
      mod2_##TYPE(z);                                               \
      CHECK_##TYPE(z);                                              \
      printf("Made it to mod1_" #TYPE "\n");                        \
      memset((void*)&z, 0, sizeof z);                               \
    } else {                                                        \
      memset((void*)&z, 0, sizeof z);                               \
      printf("should_be_true is not true for mod1_" #TYPE "\n");    \
      mod2_##TYPE(z);                                               \
    }                                                               \
  }

#define GENERATE_FOR_MODULE0(TYPE)                                  \
  extern __attribute__((noinline)) void mod1_##TYPE(TYPE z) ;       \
  void __attribute__((noinline)) mod0_##TYPE(TYPE z) ;              \
  void mod0_##TYPE(TYPE z) {                                        \
    CHECK_##TYPE(z);                                                \
    if (should_be_true) {                                           \
      mod1_##TYPE(z);                                               \
      CHECK_##TYPE(z);                                              \
      printf("Made it to mod0_" #TYPE "\n");                        \
      memset((void*)&z, 0, sizeof z);                               \
    } else {                                                        \
      memset((void*)&z, 0, sizeof z);                               \
      printf("should_be_true is not true for mod0_" #TYPE "\n");    \
      mod1_##TYPE(z);                                               \
    }                                                               \
  }                                                                 \
  void  __attribute__((noinline)) test_##TYPE(void);                \
  void test_##TYPE(void) {                                          \
    TYPE z = k##TYPE;                                               \
    CHECK_##TYPE(z);                                                \
    if (should_be_true) {                                           \
      mod0_##TYPE(z);                                               \
      CHECK_##TYPE(z);                                              \
      printf("Made it through test_" #TYPE "\n");                   \
    } else {                                                        \
      memset((void*)&z, 0, sizeof z);                               \
      printf("should_be_true is not true for test_" #TYPE "\n");    \
      mod0_##TYPE(z);                                               \
    }                                                               \
  }


#undef DO_FOR_TYPE
#if defined(MODULE0)
#define DO_FOR_TYPE GENERATE_FOR_MODULE0
#elif defined(MODULE1)
#define DO_FOR_TYPE GENERATE_FOR_MODULE1
#elif defined(MODULE2)
#define DO_FOR_TYPE GENERATE_FOR_MODULE2
#elif defined(MODULE3)
#define DO_FOR_TYPE GENERATE_FOR_MODULE3
#else
#error "Must define MODULE0. or MODULE1, 2 or 3 in preprocessor!"
#endif  /* defined(MODULE0) */

#include "native_client/tests/callingconv_case_by_case/for_each_type.h"
#undef DO_FOR_TYPE

/* Place Main in Module 0 */
#if defined(MODULE0)
int main(int argc, char* argv[]) {
  should_be_true = 0;

  /* This should always be true when running this test. */
  if (argc != 55) {
    should_be_true = 1;
  }

  /* Set NOT_DECLARING_DEFINING to tell for_each_type.h that this is not
   * for declarations and definition specific stuff like extern "C".
   */
#define NOT_DECLARING_DEFINING 1
#define CALL_TEST(TYPE)                         \
  test_##TYPE();

#define DO_FOR_TYPE CALL_TEST
#include "native_client/tests/callingconv_case_by_case/for_each_type.h"
#undef DO_FOR_TYPE
#undef NOT_DECLARING_DEFINING

  return 0;
}
#endif /* defined(MODULE0) */
