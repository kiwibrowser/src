/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_DRIVER_ELF_LOAD_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_DRIVER_ELF_LOAD_H_

#include <vector>

#include "native_client/src/include/portability.h"


namespace elf_load {

typedef std::vector<uint8_t> Image;

// Hypothetically reading whole ELF file to memory can cause problems with huge
// amounts of debug info, but unless it actually happens this approach is used
// for simplicity.
void ReadImage(const char *filename, Image *image);


enum Architecture {
  X86_32,
  X86_64,
  ARM
};


// Given valid elf image, returns architecture (x86-32, x86-64 or ARM).
// Note that NaCl allows to have 64-bit code in ELF32 file, so architecture is
// determined independently of ELF bitness.
Architecture GetElfArch(const Image &image);


struct Segment {
  const uint8_t *data;
  uint32_t size;
  uint32_t vaddr;
};


Segment GetElfTextSegment(const Image &image);

}  // namespace elf_load

#endif
