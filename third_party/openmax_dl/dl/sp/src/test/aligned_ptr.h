/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_ARM_FFT_TEST_ALIGNED_PTR_H_
#define WEBRTC_ARM_FFT_TEST_ALIGNED_PTR_H_

/*
 * Simple aligned pointer structure to get provide aligned pointers
 * that can be freed.
 */
struct AlignedPtr {
  void* raw_pointer_;
  void* aligned_pointer_;
};

/*
 * Allocate an aligned pointer to an area of size |bytes| bytes.  The
 * pointer is aligned to |alignment| bytes, which MUST be a power of
 * two.  The aligned pointer itself is in the aligned_pointer_ slot of
 * the structure.
 */
struct AlignedPtr* AllocAlignedPointer(int alignment, int bytes);

/*
 * Free the memory allocated for the aligned pointer, including the
 * object itself.
 */
void FreeAlignedPointer(struct AlignedPtr* pointer);

#endif
