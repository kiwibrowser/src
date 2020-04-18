// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_INVALIDATION_REASON_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_INVALIDATION_REASON_H_

#include <iosfwd>
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

enum class PaintInvalidationReason : unsigned {
  kNone,
  kIncremental,
  kRectangle,
  // The following reasons will all cause full paint invalidation.
  kFull,  // Any unspecified reason of full invalidation.
  kStyle,
  kGeometry,  // Layout or visual geometry change.
  kCompositing,
  kAppeared,
  kDisappeared,
  kScroll,
  kScrollControl,  // scroll bars, scroll corner, etc.
  kSelection,
  kOutline,
  kSubtree,
  kSVGResource,
  kBackground,
  kBackgroundOnScrollingContentsLayer,
  kCaret,
  kDocumentMarker,
  kImage,
  kChunkAppeared,
  kChunkDisappeared,
  kChunkUncacheable,
  kChunkReordered,
  kPaintProperty,
  kForTesting,
  // kDelayedFull means that kFull is needed in order to fully paint the
  // content, but that painting of the object can be delayed until a future
  // frame. This can be the case for an object whose content is not visible to
  // the user.
  kDelayedFull,

  kMax = kDelayedFull
};

PLATFORM_EXPORT const char* PaintInvalidationReasonToString(
    PaintInvalidationReason);

inline bool IsFullPaintInvalidationReason(PaintInvalidationReason reason) {
  return reason >= PaintInvalidationReason::kFull;
}

inline bool IsImmediateFullPaintInvalidationReason(
    PaintInvalidationReason reason) {
  return IsFullPaintInvalidationReason(reason) &&
         reason != PaintInvalidationReason::kDelayedFull;
}

PLATFORM_EXPORT std::ostream& operator<<(std::ostream&,
                                         PaintInvalidationReason);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_INVALIDATION_REASON_H_
