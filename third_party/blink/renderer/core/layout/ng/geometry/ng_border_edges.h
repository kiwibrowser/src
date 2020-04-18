// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGBorderEdges_h
#define NGBorderEdges_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/text/writing_mode.h"

namespace blink {

// Which border edges should be painted. Due to fragmentation one or more may
// be skipped.
struct CORE_EXPORT NGBorderEdges {
  unsigned block_start : 1;
  unsigned line_right : 1;
  unsigned block_end : 1;
  unsigned line_left : 1;

  NGBorderEdges()
      : block_start(true), line_right(true), block_end(true), line_left(true) {}
  NGBorderEdges(bool block_start,
                bool line_right,
                bool block_end,
                bool line_left)
      : block_start(block_start),
        line_right(line_right),
        block_end(block_end),
        line_left(line_left) {}

  enum Physical {
    kTop = 1,
    kRight = 2,
    kBottom = 4,
    kLeft = 8,
    kAll = kTop | kRight | kBottom | kLeft
  };
  static NGBorderEdges FromPhysical(unsigned, WritingMode);
  unsigned ToPhysical(WritingMode) const;
};

}  // namespace blink

#endif  // NGBorderEdges_h
