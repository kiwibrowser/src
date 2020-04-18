// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_STYLE_DIFFERENCE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_STYLE_DIFFERENCE_H_

#include <iosfwd>
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

class StyleDifference {
  STACK_ALLOCATED();

 public:
  enum PropertyDifference {
    kTransformChanged = 1 << 0,
    kOpacityChanged = 1 << 1,
    kZIndexChanged = 1 << 2,
    kFilterChanged = 1 << 3,
    kBackdropFilterChanged = 1 << 4,
    kCSSClipChanged = 1 << 5,
    // The object needs to issue paint invalidations if it is affected by text
    // decorations or properties dependent on color (e.g., border or outline).
    kTextDecorationOrColorChanged = 1 << 6,
    // If you add a value here, be sure to update kPropertyDifferenceCount.
  };

  StyleDifference()
      : paint_invalidation_type_(kNoPaintInvalidation),
        layout_type_(kNoLayout),
        recompute_overflow_(false),
        visual_rect_update_(false),
        property_specific_differences_(0),
        scroll_anchor_disabling_property_changed_(false),
        composited_reasons_changed_(false) {}

  bool HasDifference() const {
    return paint_invalidation_type_ || layout_type_ ||
           property_specific_differences_ || recompute_overflow_ ||
           visual_rect_update_ || scroll_anchor_disabling_property_changed_ ||
           composited_reasons_changed_;
  }

  bool HasAtMostPropertySpecificDifferences(
      unsigned property_differences) const {
    return !paint_invalidation_type_ && !layout_type_ &&
           !(property_specific_differences_ & ~property_differences);
  }

  bool NeedsFullPaintInvalidation() const {
    return paint_invalidation_type_ != kNoPaintInvalidation;
  }

  // The object just needs to issue paint invalidations.
  bool NeedsPaintInvalidationObject() const {
    return paint_invalidation_type_ == kPaintInvalidationObject;
  }
  void SetNeedsPaintInvalidationObject() {
    DCHECK(!NeedsPaintInvalidationSubtree());
    paint_invalidation_type_ = kPaintInvalidationObject;
  }

  // The object and its descendants need to issue paint invalidations.
  bool NeedsPaintInvalidationSubtree() const {
    return paint_invalidation_type_ == kPaintInvalidationSubtree;
  }
  void SetNeedsPaintInvalidationSubtree() {
    paint_invalidation_type_ = kPaintInvalidationSubtree;
  }

  bool NeedsLayout() const { return layout_type_ != kNoLayout; }
  void ClearNeedsLayout() { layout_type_ = kNoLayout; }

  // The offset of this positioned object has been updated.
  bool NeedsPositionedMovementLayout() const {
    return layout_type_ == kPositionedMovement;
  }
  void SetNeedsPositionedMovementLayout() {
    DCHECK(!NeedsFullLayout());
    layout_type_ = kPositionedMovement;
  }

  bool NeedsFullLayout() const { return layout_type_ == kFullLayout; }
  void SetNeedsFullLayout() { layout_type_ = kFullLayout; }

  bool NeedsRecomputeOverflow() const { return recompute_overflow_; }
  void SetNeedsRecomputeOverflow() { recompute_overflow_ = true; }

  bool NeedsVisualRectUpdate() const { return visual_rect_update_; }
  void SetNeedsVisualRectUpdate() { visual_rect_update_ = true; }

  bool TransformChanged() const {
    return property_specific_differences_ & kTransformChanged;
  }
  void SetTransformChanged() {
    property_specific_differences_ |= kTransformChanged;
  }

  bool OpacityChanged() const {
    return property_specific_differences_ & kOpacityChanged;
  }
  void SetOpacityChanged() {
    property_specific_differences_ |= kOpacityChanged;
  }

  bool ZIndexChanged() const {
    return property_specific_differences_ & kZIndexChanged;
  }
  void SetZIndexChanged() { property_specific_differences_ |= kZIndexChanged; }

  bool FilterChanged() const {
    return property_specific_differences_ & kFilterChanged;
  }
  void SetFilterChanged() { property_specific_differences_ |= kFilterChanged; }

  bool BackdropFilterChanged() const {
    return property_specific_differences_ & kBackdropFilterChanged;
  }
  void SetBackdropFilterChanged() {
    property_specific_differences_ |= kBackdropFilterChanged;
  }

  bool CssClipChanged() const {
    return property_specific_differences_ & kCSSClipChanged;
  }
  void SetCSSClipChanged() {
    property_specific_differences_ |= kCSSClipChanged;
  }

  bool TextDecorationOrColorChanged() const {
    return property_specific_differences_ & kTextDecorationOrColorChanged;
  }
  void SetTextDecorationOrColorChanged() {
    property_specific_differences_ |= kTextDecorationOrColorChanged;
  }

  bool ScrollAnchorDisablingPropertyChanged() const {
    return scroll_anchor_disabling_property_changed_;
  }
  void SetScrollAnchorDisablingPropertyChanged() {
    scroll_anchor_disabling_property_changed_ = true;
  }
  bool CompositingReasonsChanged() const { return composited_reasons_changed_; }
  void SetCompositingReasonsChanged() { composited_reasons_changed_ = true; }

 private:
  static constexpr int kPropertyDifferenceCount = 7;

  friend CORE_EXPORT std::ostream& operator<<(std::ostream&,
                                              const StyleDifference&);

  enum PaintInvalidationType {
    kNoPaintInvalidation,
    kPaintInvalidationObject,
    kPaintInvalidationSubtree,
  };
  unsigned paint_invalidation_type_ : 2;

  enum LayoutType { kNoLayout = 0, kPositionedMovement, kFullLayout };
  unsigned layout_type_ : 2;
  unsigned recompute_overflow_ : 1;
  unsigned visual_rect_update_ : 1;
  unsigned property_specific_differences_ : kPropertyDifferenceCount;
  unsigned scroll_anchor_disabling_property_changed_ : 1;
  unsigned composited_reasons_changed_ : 1;
};

CORE_EXPORT std::ostream& operator<<(std::ostream&, const StyleDifference&);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_STYLE_DIFFERENCE_H_
