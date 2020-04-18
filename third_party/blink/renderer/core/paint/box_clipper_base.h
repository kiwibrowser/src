// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_CLIPPER_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_CLIPPER_BASE_H_

#include "base/optional.h"
#include "third_party/blink/renderer/platform/graphics/paint/scoped_paint_chunk_properties.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class DisplayItemClient;
class FragmentData;
struct PaintInfo;

class BoxClipperBase {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 protected:
  void InitializeScopedClipProperty(const FragmentData*,
                                    const DisplayItemClient&,
                                    const PaintInfo&);

  base::Optional<ScopedPaintChunkProperties> scoped_clip_property_;
};

}  // namespace blink

#endif  // BoxClipper_h
