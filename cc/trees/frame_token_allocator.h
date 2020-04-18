// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_FRAME_TOKEN_ALLOCATOR_H_
#define CC_TREES_FRAME_TOKEN_ALLOCATOR_H_

#include <stdint.h>

#include "base/macros.h"
#include "cc/cc_export.h"

namespace cc {

// For some compositor frame submissions, there is additional work which a frame
// embedder wishes to perform only after the frame has been processed by the
// display compositor.
//
// For this a FrameToken is sent both with the compositor frame submission, as
// well as with messages to the embedder.
//
// However for any given frame there are multiple possible sources which may
// wish to increment the FrameToken. FrameTokenAllocator is a shared source of
// these tokens, only ever increasing the token once during a given frame
// submission.
class CC_EXPORT FrameTokenAllocator {
 public:
  FrameTokenAllocator() = default;
  virtual ~FrameTokenAllocator() = default;

  // During frame submission the first call to this allocates and returns a new
  // frame token. All subsequent calls return the current frame token.
  uint32_t GetOrAllocateFrameToken();

  // Gets the token for the current frame submission, signifying the end of
  // frame submission. Or 0 is no token was allocated. The next call to
  // GetOrAllocateFrameToken will lead to the generation of a new frame token.
  uint32_t GetFrameTokenForSubmission();

 private:
  // True if a frame token is allocated during the current frame submission.
  bool frame_token_allocated_ = false;

  // The current frame token.
  uint32_t frame_token_ = 0;

  DISALLOW_COPY_AND_ASSIGN(FrameTokenAllocator);
};

}  // namespace cc

#endif  // CC_TREES_FRAME_TOKEN_ALLOCATOR_H_
