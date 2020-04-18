/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <nacl/nacl_random.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/tests/inbrowser_test_runner/test_runner.h"

#ifdef _NEWLIB_VERSION
/* TODO(sbc): Remove these once they get added to stdlib.h */
long int random(void);
void srandom(unsigned int seed);
#endif

int TestRand(void) {
  /*
   * Simply test that rand/srand and random/srandom are linkable
   * and callable
   */
  random();
  srandom(123);
  random();

  rand();
  srand(123);
  rand();

  return 0;
}

int TestSecureRandom(void) {
  int result = 0;

  uint8_t byte1 = 0;
  uint8_t byte2 = 0;
  size_t nread;
  int error = nacl_secure_random(&byte1, sizeof(byte1), &nread);
  if (error != 0) {
    fprintf(stderr, "get_random_bytes failed for size %u: %s\n",
            sizeof(byte1), strerror(error));
    result = 1;
  } else if (nread != sizeof(byte1)) {
    fprintf(stderr, "get_random_bytes read %u != %u\n", nread, sizeof(byte1));
    result = 1;
  } else {
    error = nacl_secure_random(&byte2, sizeof(byte2), &nread);
    if (error != 0) {
      fprintf(stderr, "get_random_bytes failed for size %u: %s\n",
              sizeof(byte2), strerror(error));
      result = 1;
    } else if (nread != sizeof(byte2)) {
      fprintf(stderr, "get_random_bytes read %u != %u\n", nread, sizeof(byte2));
      result = 1;
    }

    /*
     * Reading the same byte value twice is not really that unlikely.  So
     * we don't test the values.  We've just tested that the code doesn't
     * freak out for single-byte reads.
     */
  }

  int int1 = 0;
  int int2 = 0;
  error = nacl_secure_random(&int1, sizeof(int1), &nread);
  if (error != 0) {
    fprintf(stderr, "get_random_bytes failed for size %u: %s\n",
            sizeof(int1), strerror(error));
    result = 1;
  } else if (nread != sizeof(int1)) {
    fprintf(stderr, "get_random_bytes read %u != %u\n", nread, sizeof(int1));
    result = 1;
  } else {
    error = nacl_secure_random(&int2, sizeof(int2), &nread);
    if (error != 0) {
      fprintf(stderr, "get_random_bytes failed for size %u: %s\n",
              sizeof(int2), strerror(error));
      result = 1;
    } else if (nread != sizeof(int2)) {
      fprintf(stderr, "get_random_bytes read %u != %u\n", nread, sizeof(int2));
      result = 1;
    } else {
      if (int1 == int2) {
        fprintf(stderr, "Read the same %u-byte value twice!  (%#x)\n",
                sizeof(int1), int1);
        result = 1;
      }
      /*
       * We got two different values, so they must be random!
       * (No, that's not really true.  But probably close enough
       * with a 32-bit value.)
       */
    }
  }

  /*
   * Calling nacl_secure_random_init() is no longer required, but we
   * provide it for compatibility.  It should always return 0 for success.
   */
  int init_result = nacl_secure_random_init();
  ASSERT_EQ(init_result, 0);

  return result;
}

int main(void) {
  int rtn = RunTests(TestSecureRandom);
  if (rtn)
    return rtn;
  return RunTests(TestRand);
}
