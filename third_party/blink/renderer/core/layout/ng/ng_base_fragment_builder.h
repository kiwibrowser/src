// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGBaseFragmentBuilder_h
#define NGBaseFragmentBuilder_h

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/ng_style_variant.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"
#include "third_party/blink/renderer/platform/text/writing_mode.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class ComputedStyle;

class CORE_EXPORT NGBaseFragmentBuilder {
  STACK_ALLOCATED();
 public:
  virtual ~NGBaseFragmentBuilder();

  const ComputedStyle& Style() const {
    DCHECK(style_);
    return *style_;
  }
  NGBaseFragmentBuilder& SetStyleVariant(NGStyleVariant);
  NGBaseFragmentBuilder& SetStyle(scoped_refptr<const ComputedStyle>,
                                  NGStyleVariant);

  WritingMode GetWritingMode() const { return writing_mode_; }
  TextDirection Direction() const { return direction_; }

 protected:
  NGBaseFragmentBuilder(scoped_refptr<const ComputedStyle>,
                        WritingMode,
                        TextDirection);
  NGBaseFragmentBuilder(WritingMode, TextDirection);

 private:
  scoped_refptr<const ComputedStyle> style_;
  WritingMode writing_mode_;
  TextDirection direction_;

 protected:
  NGStyleVariant style_variant_;
};

}  // namespace blink

#endif  // NGBaseFragmentBuilder
