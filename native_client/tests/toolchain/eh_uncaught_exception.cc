/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>

#include <exception>

#include "native_client/src/include/nacl_compiler_annotations.h"


// This test checks that C++ destructors are not called when an
// uncaught exception occurs.
//
// The C++ standard makes it optional whether destructors are called
// in this situation.  It is more convenient for debugging if
// destructors are not called, because unwinding the stack to call
// them would lose the context of where the uncaught exception was
// thrown from.

static void terminate_handler() {
  fprintf(stderr, "std::terminate() called as expected\n");
  // Indicate success.
  exit(55);
}

class ObjWithDtor {
 public:
  ~ObjWithDtor() {
    fprintf(stderr, "Error: Destructor called unexpectedly\n");
    exit(1);
  }
};

int main() {
  std::set_terminate(terminate_handler);
  ObjWithDtor obj;
  UNREFERENCED_PARAMETER(obj);
  throw 1;
  return 1;
}
