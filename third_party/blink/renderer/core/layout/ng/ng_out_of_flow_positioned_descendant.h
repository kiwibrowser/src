// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGOutOfFlowPositionedDescendant_h
#define NGOutOfFlowPositionedDescendant_h

#include "third_party/blink/renderer/core/core_export.h"

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_static_position.h"
#include "third_party/blink/renderer/core/layout/ng/ng_block_node.h"

namespace blink {

// An out-of-flow positioned-descendant is an element with the style "postion:
// absolute" or "position: fixed" which hasn't been bubbled up to its
// containing block yet, e.g. an element with "position: relative". As soon as
// a descendant reaches its containing block, it gets placed, and doesn't bubble
// up the tree.
//
// This needs its static position [1] to be placed correcting in its containing
// block.
//
// [1] https://www.w3.org/TR/CSS2/visudet.html#abs-non-replaced-width
struct CORE_EXPORT NGOutOfFlowPositionedDescendant {
  NGBlockNode node;
  NGStaticPosition static_position;
  LayoutObject* inline_container;
  NGOutOfFlowPositionedDescendant(
      NGBlockNode node_param,
      NGStaticPosition static_position_param,
      LayoutObject* inline_container_param = nullptr)
      : node(node_param),
        static_position(static_position_param),
        inline_container(inline_container_param) {}
};

}  // namespace blink

#endif  // NGOutOfFlowPositionedDescendant_h
