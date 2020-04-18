// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper_transform_cache.h"

#include "third_party/blink/renderer/platform/graphics/paint/transform_paint_property_node.h"

namespace blink {

// All transform caches invalidate themselves by tracking a local cache
// generation, and invalidating their cache if their cache generation disagrees
// with s_global_generation.
unsigned GeometryMapperTransformCache::s_global_generation;

void GeometryMapperTransformCache::ClearCache() {
  s_global_generation++;
}

// Computes flatten(m) ^ -1, return true if the inversion succeeded.
static bool InverseProjection(const TransformationMatrix& m,
                              TransformationMatrix& out) {
  out = m;
  out.FlattenTo2d();
  if (!out.IsInvertible())
    return false;
  out = out.Inverse();
  return true;
}

void GeometryMapperTransformCache::Update(
    const TransformPaintPropertyNode& node) {
  DCHECK_NE(cache_generation_, s_global_generation);
  cache_generation_ = s_global_generation;

  if (!node.Parent()) {
    to_screen_.MakeIdentity();
    to_screen_is_invertible_ = true;
    projection_from_screen_.MakeIdentity();
    projection_from_screen_is_valid_ = true;
    plane_root_ = &node;
    to_plane_root_.MakeIdentity();
    from_plane_root_.MakeIdentity();
    return;
  }

  const GeometryMapperTransformCache& parent =
      node.Parent()->GetTransformCache();

  TransformationMatrix local = node.Matrix();
  local.ApplyTransformOrigin(node.Origin());

  to_screen_ = parent.to_screen_;
  if (node.FlattensInheritedTransform())
    to_screen_.FlattenTo2d();
  to_screen_.Multiply(local);
  to_screen_is_invertible_ = to_screen_.IsInvertible();
  projection_from_screen_is_valid_ =
      InverseProjection(to_screen_, projection_from_screen_);

  if (!local.IsFlat() || !local.IsInvertible()) {
    plane_root_ = &node;
    to_plane_root_.MakeIdentity();
    from_plane_root_.MakeIdentity();
  } else {  // (local.IsFlat() && local.IsInvertible())
    plane_root_ = parent.plane_root_;
    to_plane_root_ = parent.to_plane_root_;
    to_plane_root_.Multiply(local);
    from_plane_root_ = local.Inverse();
    from_plane_root_.Multiply(parent.from_plane_root_);
  }
}

}  // namespace blink
