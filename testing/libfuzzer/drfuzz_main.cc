// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

// Provide main for running fuzzer tests with Dr. Fuzz.
int main(int argc, char **argv)
{
  static const size_t kFuzzInputMaxSize = 8;
  uint8_t* fuzz_input = new uint8_t[kFuzzInputMaxSize]();
  // The buffer and size arguments can be changed by Dr. Fuzz.
  int result = LLVMFuzzerTestOneInput(fuzz_input, kFuzzInputMaxSize);
  delete[] fuzz_input;
  return result;
}
