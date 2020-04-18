// Copyright 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>

#include "native_client/tests/benchmark/framework.h"

int main(int argc, char **argv) {
  const char *description = argc >= 2 ? argv[1] : "time";
  setlinebuf(stdout);
  return BenchmarkSuite::Run(description, NULL, NULL);
}
