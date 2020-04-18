// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_INLINE_BOX_TRAVERSAL_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_INLINE_BOX_TRAVERSAL_H_

// TODO(xiaochengh): Rename this file to |bidi_adjustment.h|

#include "third_party/blink/renderer/core/editing/forward.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class InlineBox;
struct InlineBoxPosition;
struct NGCaretPosition;
enum class UnicodeBidi : unsigned;

class BidiAdjustment final {
  STATIC_ONLY(BidiAdjustment);

 public:
  // Function to be called at the end of caret position resolution, adjusting
  // the result in bidi text runs.
  // TODO(xiaochengh): Eliminate |unicode_bidi| from parameters.
  static InlineBoxPosition AdjustForCaretPositionResolution(
      const InlineBoxPosition&,
      UnicodeBidi unicode_bidi);
  static NGCaretPosition AdjustForCaretPositionResolution(
      const NGCaretPosition&);

  // Function to be called at the end of hit tests, adjusting the result in bidi
  // text runs.
  static InlineBoxPosition AdjustForHitTest(const InlineBoxPosition&);
  static NGCaretPosition AdjustForHitTest(const NGCaretPosition&);

  // Function to be called at the end of creating a range selection by mouse
  // dragging, ensuring that the created range selection matches the dragging
  // even with bidi adjustment.
  // TODO(editing-dev): Eliminate |VisiblePosition| from this function.
  static SelectionInFlatTree AdjustForRangeSelection(
      const VisiblePositionInFlatTree&,
      const VisiblePositionInFlatTree&);
};

// This class provides common traveral functions on list of |InlineBox|.
// TODO(xiaochengh): Code using InlineBoxTraversal should be merged into the .cc
// file and templatized to share code with NG bidi traversal.
class InlineBoxTraversal final {
  STATIC_ONLY(InlineBoxTraversal);

 public:
  // TODO(yosin): We should take |bidi_level| from |InlineBox::BidiLevel()|,
  // once all call sites satisfy it.

  // Traverses left/right from |box|, and returns the first box with bidi level
  // less than or equal to |bidi_level| (excluding |box| itself). Returns
  // |nullptr| when such a box doesn't exist.
  static const InlineBox* FindLeftBidiRun(const InlineBox& box,
                                          unsigned bidi_level);
  static const InlineBox* FindRightBidiRun(const InlineBox& box,
                                           unsigned bidi_level);

  // Traverses left/right from |box|, and returns the last non-linebreak box
  // with bidi level greater than |bidi_level| (including |box| itself).
  static const InlineBox& FindLeftBoundaryOfBidiRunIgnoringLineBreak(
      const InlineBox& box,
      unsigned bidi_level);
  static const InlineBox& FindRightBoundaryOfBidiRunIgnoringLineBreak(
      const InlineBox& box,
      unsigned bidi_level);

  // Traverses left/right from |box|, and returns the last box with bidi level
  // greater than or equal to |bidi_level| (including |box| itself).
  static const InlineBox& FindLeftBoundaryOfEntireBidiRun(const InlineBox& box,
                                                          unsigned bidi_level);
  static const InlineBox& FindRightBoundaryOfEntireBidiRun(const InlineBox& box,
                                                           unsigned bidi_level);

  // Variants of the above two where line break boxes are ignored.
  static const InlineBox& FindLeftBoundaryOfEntireBidiRunIgnoringLineBreak(
      const InlineBox&,
      unsigned bidi_level);
  static const InlineBox& FindRightBoundaryOfEntireBidiRunIgnoringLineBreak(
      const InlineBox&,
      unsigned bidi_level);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_INLINE_BOX_TRAVERSAL_H_
