// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_base_fragment_builder.h"

#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

NGBaseFragmentBuilder::NGBaseFragmentBuilder(
    scoped_refptr<const ComputedStyle> style,
    WritingMode writing_mode,
    TextDirection direction)
    : style_(std::move(style)),
      writing_mode_(writing_mode),
      direction_(direction),
      style_variant_(NGStyleVariant::kStandard) {
  DCHECK(style_);
}

NGBaseFragmentBuilder::NGBaseFragmentBuilder(WritingMode writing_mode,
                                             TextDirection direction)
    : writing_mode_(writing_mode), direction_(direction) {}

NGBaseFragmentBuilder::~NGBaseFragmentBuilder() = default;

NGBaseFragmentBuilder& NGBaseFragmentBuilder::SetStyleVariant(
    NGStyleVariant style_variant) {
  style_variant_ = style_variant;
  return *this;
}

NGBaseFragmentBuilder& NGBaseFragmentBuilder::SetStyle(
    scoped_refptr<const ComputedStyle> style,
    NGStyleVariant style_variant) {
  DCHECK(style);
  style_ = std::move(style);
  style_variant_ = style_variant;
  return *this;
}

}  // namespace blink
