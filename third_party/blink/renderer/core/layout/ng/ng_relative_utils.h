// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGRelativeUtils_h
#define NGRelativeUtils_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_size.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"

namespace blink {

class ComputedStyle;
struct NGLogicalOffset;

// Implements relative positioning spec:
// https://www.w3.org/TR/css-position-3/#rel-pos
// Return relative position offset as defined by style.
CORE_EXPORT NGLogicalOffset
ComputeRelativeOffset(const ComputedStyle& child_style,
                      WritingMode container_writing_mode,
                      TextDirection container_direction,
                      NGLogicalSize container_size);

}  // namespace blink

#endif  // NGRelativeUtils_h
