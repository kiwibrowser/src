// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FIND_PROPERTIES_NEEDING_UPDATE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FIND_PROPERTIES_NEEDING_UPDATE_H_

#if DCHECK_IS_ON()

#include <memory>

#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/paint/object_paint_properties.h"
#include "third_party/blink/renderer/core/paint/paint_property_tree_builder.h"

namespace blink {

// This file contains a scope class for catching cases where paint properties
// needed an update but were not marked as such. If paint properties will
// change, the object must be marked as needing a paint property update
// using LayoutObject::SetNeedsPaintPropertyUpdate() or by forcing a subtree
// update (see: PaintPropertyTreeBuilderContext::force_subtree_update).
//
// This scope class works by recording the paint property state of an object
// before rebuilding properties, forcing the properties to get updated, then
// checking that the updated properties match the original properties.

#define DUMP_PROPERTIES(original, updated)                           \
  "\nOriginal:\n"                                                    \
      << (original ? (original)->ToString().Ascii().data() : "null") \
      << "\nUpdated:\n"                                              \
      << (updated ? (updated)->ToString().Ascii().data() : "null")

#define CHECK_PROPERTY_EQ(thing, original, updated)                   \
  do {                                                                \
    DCHECK(!!original == !!updated)                                   \
        << "Property was created or deleted "                         \
        << "without " << thing << " needing a paint property update." \
        << DUMP_PROPERTIES(original, updated);                        \
    if (!!original && !!updated) {                                    \
      DCHECK(*original == *updated)                                   \
          << "Property was updated without " << thing                 \
          << " needing a paint property update."                      \
          << DUMP_PROPERTIES(original, updated);                      \
    }                                                                 \
  } while (0)

#define DCHECK_OBJECT_PROPERTY_EQ(object, original, updated)            \
  CHECK_PROPERTY_EQ("the layout object (" << object.DebugName() << ")", \
                    original, updated)

class FindObjectPropertiesNeedingUpdateScope {
 public:
  FindObjectPropertiesNeedingUpdateScope(const LayoutObject& object,
                                         const FragmentData& fragment_data,
                                         bool force_subtree_update)
      : object_(object),
        fragment_data_(fragment_data),
        needed_paint_property_update_(object.NeedsPaintPropertyUpdate()),
        needed_forced_subtree_update_(force_subtree_update),
        original_paint_offset_(fragment_data.PaintOffset()) {
    // No need to check if an update was already needed.
    if (needed_paint_property_update_ || needed_forced_subtree_update_)
      return;

    // Mark the properties as needing an update to ensure they are rebuilt.
    object.GetMutableForPainting()
        .SetOnlyThisNeedsPaintPropertyUpdateForTesting();

    if (const auto* properties = fragment_data_.PaintProperties())
      original_properties_ = properties->Clone();

    if (fragment_data_.HasLocalBorderBoxProperties()) {
      original_local_border_box_properties_ =
          std::make_unique<PropertyTreeState>(
              fragment_data_.LocalBorderBoxProperties());
    }
  }

  ~FindObjectPropertiesNeedingUpdateScope() {
    // Paint offset and paintOffsetTranslation should not change under
    // FindObjectPropertiesNeedingUpdateScope no matter if we needed paint
    // property update.
    LayoutPoint paint_offset = fragment_data_.PaintOffset();
    DCHECK_OBJECT_PROPERTY_EQ(object_, &original_paint_offset_, &paint_offset);
    const auto* object_properties = fragment_data_.PaintProperties();
    if (original_properties_ && object_properties) {
      DCHECK_OBJECT_PROPERTY_EQ(object_,
                                original_properties_->PaintOffsetTranslation(),
                                object_properties->PaintOffsetTranslation());
    }

    // No need to check if an update was already needed.
    if (needed_paint_property_update_ || needed_forced_subtree_update_)
      return;

    // If these checks fail, the paint properties changed unexpectedly. This is
    // due to missing one of these paint property invalidations:
    // 1) The LayoutObject should have been marked as needing an update with
    //    LayoutObject::setNeedsPaintPropertyUpdate().
    // 2) The PrePaintTreeWalk should have had a forced subtree update (see:
    //    PaintPropertyTreeBuilderContext::force_subtree_update).
    if (original_properties_ && object_properties) {
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->Transform(),
                                object_properties->Transform());
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->Effect(),
                                object_properties->Effect());
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->Filter(),
                                object_properties->Filter());
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->Mask(),
                                object_properties->Mask());
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->ClipPath(),
                                object_properties->ClipPath());
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->ClipPathClip(),
                                object_properties->ClipPathClip());
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->MaskClip(),
                                object_properties->MaskClip());
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->CssClip(),
                                object_properties->CssClip());
      DCHECK_OBJECT_PROPERTY_EQ(object_,
                                original_properties_->CssClipFixedPosition(),
                                object_properties->CssClipFixedPosition());
      DCHECK_OBJECT_PROPERTY_EQ(object_,
                                original_properties_->OverflowControlsClip(),
                                object_properties->OverflowControlsClip());
      DCHECK_OBJECT_PROPERTY_EQ(object_,
                                original_properties_->InnerBorderRadiusClip(),
                                object_properties->InnerBorderRadiusClip());
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->OverflowClip(),
                                object_properties->OverflowClip());
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->Perspective(),
                                object_properties->Perspective());
      DCHECK_OBJECT_PROPERTY_EQ(
          object_, original_properties_->SvgLocalToBorderBoxTransform(),
          object_properties->SvgLocalToBorderBoxTransform());
      DCHECK_OBJECT_PROPERTY_EQ(object_, original_properties_->Scroll(),
                                object_properties->Scroll());
      DCHECK_OBJECT_PROPERTY_EQ(object_,
                                original_properties_->ScrollTranslation(),
                                object_properties->ScrollTranslation());
    } else {
      DCHECK_EQ(!!original_properties_, !!object_properties)
          << " Object: " << object_.DebugName();
    }

    if (original_local_border_box_properties_ &&
        fragment_data_.HasLocalBorderBoxProperties()) {
      const auto object_border_box = fragment_data_.LocalBorderBoxProperties();
      DCHECK_OBJECT_PROPERTY_EQ(
          object_, original_local_border_box_properties_->Transform(),
          object_border_box.Transform());
      DCHECK_OBJECT_PROPERTY_EQ(object_,
                                original_local_border_box_properties_->Clip(),
                                object_border_box.Clip());
      DCHECK_OBJECT_PROPERTY_EQ(object_,
                                original_local_border_box_properties_->Effect(),
                                object_border_box.Effect());
    } else {
      DCHECK_EQ(!!original_local_border_box_properties_,
                fragment_data_.HasLocalBorderBoxProperties())
          << " Object: " << object_.DebugName();
    }

    // Restore original clean bit.
    object_.GetMutableForPainting().ClearNeedsPaintPropertyUpdateForTesting();
  }

 private:
  const LayoutObject& object_;
  const FragmentData& fragment_data_;
  bool needed_paint_property_update_;
  bool needed_forced_subtree_update_;
  LayoutPoint original_paint_offset_;
  std::unique_ptr<const ObjectPaintProperties> original_properties_;
  std::unique_ptr<const PropertyTreeState>
      original_local_border_box_properties_;
};

}  // namespace blink
#endif  // DCHECK_IS_ON()

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FIND_PROPERTIES_NEEDING_UPDATE_H_
