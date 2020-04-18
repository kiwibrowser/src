/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Simple fault injection test.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/fault_injection/fault_injection.h"

int FunctionThatMightFail(size_t ix) {
  return (int) ix+1;
}

enum ErrorCode {
  PASS,
  FAIL,
  RETAKE_PREVIOUS_GRADE,
  GOTO_JAIL_DO_NOT_PASS_GO,
};

enum ErrorCode SomeFunction(size_t ix) {
  switch (ix & 0x3) {
    case 0:
      return PASS;
    case 1:
      return FAIL;
  }
  return RETAKE_PREVIOUS_GRADE;
}

ssize_t fake_write(size_t ix) {
  return (ssize_t) ix;
}

int fake_fstat(size_t ix) {
  return (int) ix;
}

int main(int ac, char **av) {
  int                   opt;
  size_t                ix;
  size_t                limit = 10u;
  char                  *buffer;
  enum ErrorCode        err;
  static enum ErrorCode expected[4] = {
    PASS, FAIL, RETAKE_PREVIOUS_GRADE, RETAKE_PREVIOUS_GRADE
  };
  ssize_t               write_result;

  NaClLogModuleInit();

  while (-1 != (opt = getopt(ac, av, "l:v"))) {
    switch (opt) {
      case 'v':
        NaClLogIncrVerbosity();
        break;
      case 'l':
        limit = strtoul(optarg, (char **) NULL, 0);
        break;
      default:
        fprintf(stderr,
                "Usage: nacl_fi_test [-v]\n");
        return 1;
    }
  }
  NaClFaultInjectionModuleInit();

  for (ix = 0; ix < limit; ++ix) {
    printf("%d\n", NACL_FI("test", FunctionThatMightFail(ix), -1));
    buffer = NACL_FI("alloc", malloc(ix+1), NULL);
    if (NULL == buffer) {
      printf("allocation for %"NACL_PRIdS" bytes failed\n", ix+1);
    } else {
      free(buffer);
      buffer = NULL;
    }
    err = NACL_FI_VAL("ret", enum ErrorCode, SomeFunction(ix));
    if (err != expected[ix & 0x3]) {
      printf("Unexpected return %d, expected %d\n",
             err, expected[ix & 0x3]);
    }
    if (-1 == NACL_FI_SYSCALL("fstat", fake_fstat(ix))) {
      printf("fstat failed, errno %d\n", errno);
    }
    if (-1 == (write_result =
               NACL_FI_TYPED_SYSCALL("write", ssize_t, fake_write(ix)))) {
      printf("write failed, errno %d\n", errno);
    } else if (write_result != (ssize_t) ix) {
      printf("unexpected write count %"NACL_PRIdS", expected %"NACL_PRIuS"\n",
             write_result, ix);
    }
  }
  return 0;
}
