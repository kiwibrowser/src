/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Tests if the ABI for returning structures matches.
 *
 * We have 4 modules, two compiled by one compiler and two by the other.
 * CC1: MODULE0 and MODULE3, CC2: MODULE1 and MODULE2
 * Overall structure is this:
 * MODULE 1                             ||   MODULE 2
 * main():
 *   test_type()
 *    ^^--return/check--^^
 *   mod0_type()  <--return/check--            mod1_type()
 *                                            ^^--return/check--^^
 *   mod3_type()  --return/check-->            mod2_type()
 *
 * So, CC1 returns stuff to CC2 -> CC2 -> CC1 -> CC1
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
  TYPE mod3_##TYPE(void) __attribute__((noinline));                 \
  TYPE mod3_##TYPE(void) {                                          \
    if (should_be_true) {                                           \
      TYPE z = k##TYPE;                                             \
      CHECK_##TYPE(z);                                              \
      printf("Made it to mod3_" #TYPE "\n");                        \
      return z;                                                     \
    } else {                                                        \
      TYPE z;                                                       \
      printf("should_be_true is not true for mod3_" #TYPE "\n");    \
      memset((void*)&z, 0, sizeof z);                               \
      return z;                                                     \
    }                                                               \
  }

#define GENERATE_FOR_MODULE2(TYPE)                                  \
  extern TYPE mod3_##TYPE(void) __attribute__((noinline));          \
  TYPE mod2_##TYPE(void) __attribute__((noinline));                 \
  TYPE mod2_##TYPE(void) {                                          \
    if (should_be_true) {                                           \
      TYPE z = mod3_##TYPE();                                       \
      CHECK_##TYPE(z);                                              \
      printf("Made it to mod2_" #TYPE "\n");                        \
      return z;                                                     \
    } else {                                                        \
      TYPE z;                                                       \
      printf("should_be_true is not true for mod2_" #TYPE "\n");    \
      memset((void*)&z, 0, sizeof z);                               \
      return z;                                                     \
    }                                                               \
  }


#define GENERATE_FOR_MODULE1(TYPE)                                  \
  extern TYPE mod2_##TYPE(void) __attribute__((noinline));          \
  TYPE mod1_##TYPE(void) __attribute__((noinline));                 \
  TYPE mod1_##TYPE(void) {                                          \
    if (should_be_true) {                                           \
      TYPE z = mod2_##TYPE();                                       \
      CHECK_##TYPE(z);                                              \
      printf("Made it to mod1_" #TYPE "\n");                        \
      return z;                                                     \
    } else {                                                        \
      TYPE z;                                                       \
      printf("should_be_true is not true for mod1_" #TYPE "\n");    \
      memset((void*)&z, 0, sizeof z);                               \
      return z;                                                     \
    }                                                               \
  }

#define GENERATE_FOR_MODULE0(TYPE)                                  \
  extern TYPE mod1_##TYPE(void) __attribute__((noinline));          \
  TYPE mod0_##TYPE(void) __attribute__((noinline));                 \
  TYPE mod0_##TYPE(void) {                                          \
    if (should_be_true) {                                           \
      TYPE z = mod1_##TYPE();                                       \
      CHECK_##TYPE(z);                                              \
      printf("Made it to mod0_" #TYPE "\n");                        \
      return z;                                                     \
    } else {                                                        \
      TYPE z;                                                       \
      printf("should_be_true is not true for mod0_" #TYPE "\n");    \
      memset((void*)&z, 0, sizeof z);                               \
      return z;                                                     \
    }                                                               \
  }                                                                 \
  void test_##TYPE(void) {                                          \
    TYPE z;                                                         \
    z = mod0_##TYPE();                                              \
    CHECK_##TYPE(z);                                                \
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
  /* Trick inliners by making calls only happen based on external values. */
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
#endif /* defined(MODULE1) */
