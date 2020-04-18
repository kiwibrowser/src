// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/frame_token_allocator.h"

namespace cc {

uint32_t FrameTokenAllocator::GetOrAllocateFrameToken() {
  if (frame_token_allocated_)
    return frame_token_;
  frame_token_allocated_ = true;

  // TODO(jonross) we will want to have a wrapping strategy to handle overflow.
  // We will also want to confirm this looping behaviour in processes which
  // receive the token.
  return ++frame_token_;
}

uint32_t FrameTokenAllocator::GetFrameTokenForSubmission() {
  uint32_t result = frame_token_allocated_ ? frame_token_ : 0;
  frame_token_allocated_ = false;
  return result;
}

}  // namespace cc
