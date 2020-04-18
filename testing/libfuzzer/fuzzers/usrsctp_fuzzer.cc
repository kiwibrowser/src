// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "third_party/usrsctp/usrsctplib/usrsctplib/usrsctp.h"

static int ignore1(void* addr,
                   void* data,
                   size_t length,
                   uint8_t tos,
                   uint8_t set_df) {
  return 0;
};

static void ignore2(const char* format, ...) {};

struct Environment {
  Environment() {
    usrsctp_init(0, ignore1, ignore2);
  }
};

Environment* env = new Environment();

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  usrsctp_conninput(nullptr, data, size, 0);
  return 0;
}
