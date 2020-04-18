/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "native_client/src/include/elf.h"
#include "native_client/src/include/elf_constants.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/driver/elf_load.h"
#include "native_client/src/trusted/validator_ragel/validator.h"


Bool ProcessError(
    const uint8_t *begin, const uint8_t *end,
    uint32_t validation_info, void *user_data_ptr) {
  UNREFERENCED_PARAMETER(begin);
  UNREFERENCED_PARAMETER(end);
  UNREFERENCED_PARAMETER(user_data_ptr);
  if (validation_info & (VALIDATION_ERRORS_MASK | BAD_JUMP_TARGET))
    return FALSE;
  else
    return TRUE;
}


int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage:\n");
    printf("    validator_benchmark <nexe> <number of repetitions>\n");
    exit(1);
  }
  const char *input_file = argv[1];
  int repetitions = atoi(argv[2]);
  CHECK(repetitions > 0);

  printf("Validating %s %d times ...\n", input_file, repetitions);

  elf_load::Image image;
  elf_load::ReadImage(input_file, &image);

  elf_load::Architecture architecture = elf_load::GetElfArch(image);
  elf_load::Segment segment = elf_load::GetElfTextSegment(image);

  if (segment.size % kBundleSize != 0) {
    printf("Text segment size (0x%" NACL_PRIx32 ") is not "
           "multiple of bundle size.\n",
           segment.size);
    exit(1);
  }

  Bool result = FALSE;

  clock_t start = clock();
  for (int i = 0; i < repetitions; i++) {
    switch (architecture) {
      case elf_load::X86_32:
        result = ValidateChunkIA32(
            segment.data, segment.size,
            0, &kFullCPUIDFeatures,
            ProcessError, NULL);
        break;
      case elf_load::X86_64:
        result = ValidateChunkAMD64(
            segment.data, segment.size,
            0, &kFullCPUIDFeatures,
            ProcessError, NULL);
        break;
      case elf_load::ARM:
        CHECK(false);
    }
  }

  if (result)
    printf("Valid.\n");
  else
    printf("Invalid.\n");

  float seconds = (float)(clock() - start) / CLOCKS_PER_SEC;
  printf("It took %.3fs", seconds);

  if (seconds > 1e-6)
    printf(" (%.3f MB/s)", segment.size / seconds * repetitions / (1<<20));

  printf("\n");

  return result ? 0 : 1;
}
