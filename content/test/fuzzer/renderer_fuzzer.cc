// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Fuzzer for content/renderer

#include <stddef.h>
#include <stdint.h>
#include <memory>

#include "content/test/fuzzer/fuzzer_support.h"

namespace content {

static Env* env = nullptr;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Environment has to be initialized in the same thread.
  if (env == nullptr)
    env = new Env();

  env->adapter->LoadHTML(std::string(reinterpret_cast<const char*>(data), size),
                         "http://www.example.org");
  return 0;
}

}  // namespace content
