/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sel_main.h"

int main(int argc,
         char **argv) {
  return NaClSelLdrMain(argc, argv);
}
