/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  char *outp;
  /* Try a few alignments, some of them might be the standard malloc align.
   * They should all be powers of 2 and be a multiple of sizeof(void*).
   */
  size_t align_to_test[] = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 0};
  size_t sizes_to_test[] = {1, 2, 4, 8, 10, 16, 32, 64, 128, 256, 512, 1024, 0};
  int i = 0;
  int j = 0;

  while (align_to_test[i] != 0) {
    j = 0;
    while (sizes_to_test[j] != 0) {
      int err = posix_memalign((void **)&outp,
                               align_to_test[i],
                               sizes_to_test[j] * sizeof(char));
      if (err) {
        fprintf(stderr,
                "posix_memalign failed with err %d (EINVAL=%d,ENOMEM=%d)\n",
                err, EINVAL, ENOMEM);
        fprintf(stderr, "Input params were align=%d, size=%d\n",
                align_to_test[i], sizes_to_test[j]);
        return 1;
      }
      if ((size_t)outp % align_to_test[i] != 0) {
        fprintf(stderr, "posix_memalign failed to align to %zu: ptr=%p\n",
                align_to_test[i], (void *) outp);
        return 1;
      }
      free(outp);
      j++;
    }
    i++;
  }

  /* Check that a non-power of 2 alignment fails. */
  if (posix_memalign((void**)&outp, 7, 20 * sizeof(char)) != EINVAL) {
    fprintf(stderr, "posix_memaligned failed to return EINVAL for non-pow2!\n");
    return 1;
  }

  /* Check that smaller than sizeof(void*) alignment fails. */
  if (posix_memalign((void**)&outp, sizeof(void*) - 1,
                     20 * sizeof(char)) != EINVAL) {
    fprintf(stderr, "posix_memaligned failed to return EINVAL for non-pow2!\n");
    return 1;
  }

  return 0;
}
