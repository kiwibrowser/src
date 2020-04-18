// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/iterators/backwards_text_buffer.h"

namespace blink {

const UChar* BackwardsTextBuffer::Data() const {
  return BufferEnd() - Size();
}

UChar* BackwardsTextBuffer::CalcDestination(size_t length) {
  DCHECK_LE(Size() + length, Capacity());
  return BufferEnd() - Size() - length;
}

void BackwardsTextBuffer::ShiftData(size_t old_capacity) {
  DCHECK_LE(old_capacity, Capacity());
  DCHECK_LE(Size(), old_capacity);
  std::copy_backward(BufferBegin() + old_capacity - Size(),
                     BufferBegin() + old_capacity, BufferEnd());
}

}  // namespace blink
