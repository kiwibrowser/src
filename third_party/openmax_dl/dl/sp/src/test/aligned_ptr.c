/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "dl/sp/src/test/aligned_ptr.h"

#include <stdlib.h>

/* |alignment| is the byte alignment and MUST be a power of two. */
struct AlignedPtr* AllocAlignedPointer(int alignment, int bytes) {
  struct AlignedPtr* aligned_ptr;
  unsigned long raw_address;
    
  aligned_ptr = (struct AlignedPtr*) malloc(sizeof(*aligned_ptr));
  aligned_ptr->raw_pointer_ = malloc(bytes + alignment);
  raw_address = (unsigned long) aligned_ptr->raw_pointer_;
  raw_address = (raw_address + alignment - 1) & ~(alignment - 1);
  aligned_ptr->aligned_pointer_ = (void*) raw_address;

  return aligned_ptr;
}

void FreeAlignedPointer(struct AlignedPtr* pointer) {
  free(pointer->raw_pointer_);
  free(pointer);
}
