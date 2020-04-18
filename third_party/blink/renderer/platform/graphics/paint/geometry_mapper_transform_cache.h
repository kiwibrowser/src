// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_GEOMETRY_MAPPER_TRANSFORM_CACHE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_GEOMETRY_MAPPER_TRANSFORM_CACHE_H_

#include "third_party/blink/renderer/platform/platform_export.h"

#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"

namespace blink {

class TransformPaintPropertyNode;

// A GeometryMapperTransformCache hangs off a TransformPaintPropertyNode.
// It stores useful intermediate results such as screen matrix for geometry
// queries.
class PLATFORM_EXPORT GeometryMapperTransformCache {
  USING_FAST_MALLOC(GeometryMapperTransformCache);
 public:
  GeometryMapperTransformCache() = default;

  static void ClearCache();

  void UpdateIfNeeded(const TransformPaintPropertyNode& node) {
    if (cache_generation_ != s_global_generation)
      Update(node);
    DCHECK_EQ(cache_generation_, s_global_generation);
  }

  const TransformationMatrix& to_screen() const { return to_screen_; }
  bool to_screen_is_invertible() const { return to_screen_is_invertible_; }

  const TransformationMatrix& projection_from_screen() const {
    return projection_from_screen_;
  }
  bool projection_from_screen_is_valid() const {
    return projection_from_screen_is_valid_;
  }

  const TransformationMatrix& to_plane_root() const { return to_plane_root_; }
  const TransformationMatrix& from_plane_root() const {
    return from_plane_root_;
  }
  const TransformPaintPropertyNode* plane_root() const { return plane_root_; }

 private:
  void Update(const TransformPaintPropertyNode&);

  static unsigned s_global_generation;

  // The cached values here can be categorized in two logical groups:
  //
  // [ Screen Transform ]
  // to_screen : The screen matrix of the node, as defined by:
  //   to_screen = (flattens_inherited_transform ?
  //       flatten(parent.to_screen) : parent.to_screen) * local
  // to_screen_is_invertible : Whether to_screen is invertible.
  // projection_from_screen : Back projection from screen.
  //   projection_from_screen = flatten(to_screen) ^ -1
  //   Undefined if the inverse projection doesn't exist.
  //   Guaranteed to be flat.
  // projection_from_screen_is_valid : Whether projection_from_screen
  //   is defined.
  //
  // [ Plane Root Transform ]
  // plane_root : The oldest ancestor node such that every intermediate node
  //   in the ancestor chain has a flat and invertible local matrix. In other
  //   words, a node inherits its parent's plane_root if its local matrix is
  //   flat and invertible. Otherwise, it becomes its own plane root.
  //   For example:
  //   <xfrm id="A" matrix="rotateY(10deg)">
  //     <xfrm id="B" flatten_inherited matrix="translateX(10px)"/>
  //     <xfrm id="C" matrix="scaleX(0)">
  //       <xfrm id="D" matrix="scaleX(2)"/>
  //       <xfrm id="E" matrix="rotate(30deg)"/>
  //     </xfrm>
  //     <xfrm id="F" matrix="scaleZ(0)"/>
  //   </xfrm>
  //   A is the plane root of itself because its local matrix is 3D.
  //   B's plane root is A because its local matrix is flat.
  //   C is the plane root of itself because its local matrix is non-invertible.
  //   D and E's plane root is C because their local matrix is flat.
  //   F is the plane root of itself because its local matrix is 3D and
  //     non-invertible.
  // to_plane_root : The accumulated matrix between this node and plane_root.
  //   to_plane_root = (plane_root == this) ? I : parent.to_plane_root * local
  //   Guaranteed to be flat.
  // from_plane_root :
  //   from_plane_root = to_plane_root ^ -1
  //   Guaranteed to exist because each local matrices are invertible.
  //   Guaranteed to be flat.
  // An important invariant is that
  //   flatten(to_screen) = flatten(plane_root.to_screen) * to_plane_root
  //   Proof by induction:
  //   If plane_root == this,
  //     flatten(plane_root.to_screen) * to_plane_root = flatten(to_screen) * I
  //     = flatten(to_screen)
  //   Otherwise,
  //     flatten(to_screen) = flatten((flattens_inherited_transform ?
  //         flatten(parent.to_screen) : parent.to_screen) * local)
  //     Because local is known to be flat,
  //     = flatten((flattens_inherited_transform ?
  //         flatten(parent.to_screen) : parent.to_screen) * flatten(local))
  //     Then by flatten lemma (https://goo.gl/DNKyOc),
  //     = flatten(parent.to_screen) * local
  //     = flatten(parent.plane_root.to_screen) * parent.to_plane_root * local
  //     = flatten(plane_root.to_screen) * to_plane_root
  TransformationMatrix to_screen_;
  TransformationMatrix projection_from_screen_;
  TransformationMatrix to_plane_root_;
  TransformationMatrix from_plane_root_;
  const TransformPaintPropertyNode* plane_root_ = nullptr;
  unsigned cache_generation_ = s_global_generation - 1;
  unsigned to_screen_is_invertible_ : 1;
  unsigned projection_from_screen_is_valid_ : 1;
  DISALLOW_COPY_AND_ASSIGN(GeometryMapperTransformCache);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_GEOMETRY_MAPPER_TRANSFORM_CACHE_H_
