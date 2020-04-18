// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_ITERATORS_BACKWARDS_TEXT_BUFFER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_ITERATORS_BACKWARDS_TEXT_BUFFER_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/editing/iterators/text_buffer_base.h"

namespace blink {

class CORE_EXPORT BackwardsTextBuffer final : public TextBufferBase {
  STACK_ALLOCATED();

 public:
  BackwardsTextBuffer() = default;
  const UChar* Data() const override;

 private:
  UChar* CalcDestination(size_t length) override;
  void ShiftData(size_t old_capacity) override;

  DISALLOW_COPY_AND_ASSIGN(BackwardsTextBuffer);
};

}  // namespace blink

#endif  // TextBuffer_h
