// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_CUSTOM_CUSTOM_LAYOUT_CONSTRAINTS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_CUSTOM_CUSTOM_LAYOUT_CONSTRAINTS_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

// Represents the constraints given to the layout by the parent that isn't
// encapsulated by the style, or edges.
class CustomLayoutConstraints : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  CustomLayoutConstraints(LayoutUnit fixed_inline_size)
      : fixed_inline_size_(fixed_inline_size.ToDouble()) {}
  ~CustomLayoutConstraints() override = default;

  // LayoutConstraints.idl
  double fixedInlineSize() const { return fixed_inline_size_; }

 private:
  double fixed_inline_size_;

  DISALLOW_COPY_AND_ASSIGN(CustomLayoutConstraints);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_CUSTOM_CUSTOM_LAYOUT_CONSTRAINTS_H_
