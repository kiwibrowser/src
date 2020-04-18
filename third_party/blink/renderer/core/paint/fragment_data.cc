// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/fragment_data.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"

#include "base/debug/stack_trace.h"

namespace blink {

// These are defined here because of PaintLayer dependency.

FragmentData::RareData::RareData(const LayoutPoint& location_in_backing)
    : unique_id(NewUniqueObjectId()),
      location_in_backing(location_in_backing) {}

FragmentData::RareData::~RareData() = default;

FragmentData& FragmentData::EnsureNextFragment() {
  if (!next_fragment_)
    next_fragment_ = std::make_unique<FragmentData>();
  return *next_fragment_.get();
}

FragmentData::RareData& FragmentData::EnsureRareData() {
  if (!rare_data_)
    rare_data_ = std::make_unique<RareData>(visual_rect_.Location());
  return *rare_data_;
}

void FragmentData::SetLayer(std::unique_ptr<PaintLayer> layer) {
  if (rare_data_ || layer)
    EnsureRareData().layer = std::move(layer);
}

const TransformPaintPropertyNode* FragmentData::PreTransform() const {
  if (const auto* properties = PaintProperties()) {
    if (properties->Transform())
      return properties->Transform()->Parent();
  }
  return LocalBorderBoxProperties().Transform();
}

const TransformPaintPropertyNode* FragmentData::PostScrollTranslation() const {
  if (const auto* properties = PaintProperties()) {
    if (properties->ScrollTranslation())
      return properties->ScrollTranslation();
    if (properties->Perspective())
      return properties->Perspective();
  }
  return LocalBorderBoxProperties().Transform();
}

const ClipPaintPropertyNode* FragmentData::PreClip() const {
  if (const auto* properties = PaintProperties()) {
    if (properties->ClipPathClip()) {
      // SPv1 composited clip-path has an alternative clip tree structure.
      // If the clip-path is parented by the mask clip, it is only used
      // to clip mask layer chunks, and not in the clip inheritance chain.
      if (properties->ClipPathClip()->Parent() != properties->MaskClip())
        return properties->ClipPathClip()->Parent();
    }
    if (properties->MaskClip())
      return properties->MaskClip()->Parent();
    if (properties->CssClip())
      return properties->CssClip()->Parent();
  }
  return LocalBorderBoxProperties().Clip();
}

const ClipPaintPropertyNode* FragmentData::PostOverflowClip() const {
  if (const auto* properties = PaintProperties()) {
    if (properties->OverflowClip())
      return properties->OverflowClip();
    if (properties->InnerBorderRadiusClip())
      return properties->InnerBorderRadiusClip();
  }
  return LocalBorderBoxProperties().Clip();
}

const EffectPaintPropertyNode* FragmentData::PreEffect() const {
  if (const auto* properties = PaintProperties()) {
    if (properties->Effect())
      return properties->Effect()->Parent();
    if (properties->Filter())
      return properties->Filter()->Parent();
  }
  return LocalBorderBoxProperties().Effect();
}

const EffectPaintPropertyNode* FragmentData::PreFilter() const {
  if (const auto* properties = PaintProperties()) {
    if (properties->Filter())
      return properties->Filter()->Parent();
  }
  return LocalBorderBoxProperties().Effect();
}

void FragmentData::InvalidateClipPathCache() {
  if (!rare_data_)
    return;

  rare_data_->is_clip_path_cache_valid = false;
  rare_data_->clip_path_bounding_box = base::nullopt;
  rare_data_->clip_path_path = nullptr;
}

void FragmentData::SetClipPathCache(const base::Optional<IntRect>& bounding_box,
                                    scoped_refptr<const RefCountedPath> path) {
  EnsureRareData().is_clip_path_cache_valid = true;
  rare_data_->clip_path_bounding_box = bounding_box;
  rare_data_->clip_path_path = std::move(path);
}

}  // namespace blink
