// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/iterators/text_buffer_base.h"

namespace blink {

TextBufferBase::TextBufferBase() {
  buffer_.ReserveCapacity(1024);
  buffer_.resize(Capacity());
}

void TextBufferBase::ShiftData(size_t) {}

void TextBufferBase::PushCharacters(UChar ch, size_t length) {
  if (length == 0)
    return;
  std::fill_n(EnsureDestination(length), length, ch);
}

UChar* TextBufferBase::EnsureDestination(size_t length) {
  if (size_ + length > Capacity())
    Grow(size_ + length);
  UChar* ans = CalcDestination(length);
  size_ += length;
  return ans;
}

void TextBufferBase::Grow(size_t demand) {
  size_t old_capacity = Capacity();
  buffer_.resize(demand);
  buffer_.resize(Capacity());
  ShiftData(old_capacity);
}

}  // namespace blink
