// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_HITTEST_H_
#define COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_HITTEST_H_

#include <set>

#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/service/viz_service_export.h"

namespace gfx {
class Point;
class Transform;
}  // namespace gfx

namespace cc {
class DrawQuad;
class RenderPass;
}  // namespace cc

namespace viz {

class SurfaceHittestDelegate;
class SurfaceManager;

// Performs a hittest in surface quads.
class VIZ_SERVICE_EXPORT SurfaceHittest {
 public:
  SurfaceHittest(SurfaceHittestDelegate* delegate, SurfaceManager* manager);
  ~SurfaceHittest();

  // Returns the target surface that falls underneath the provided |point|.
  // |out_query_renderer| is set to true if the result is not high confidence,
  // which means that there could be a different result when hit testing is
  // performed in the renderer process. In that case the returned SurfaceId and
  // Transform should be ignored.
  // When |out_query_renderer| is set to false the returned SurfaceId and
  // Transform can be taken as correct without further verification.
  // Also returns the |transform| to convert the |point| to the target surface's
  // space.
  SurfaceId GetTargetSurfaceAtPoint(const SurfaceId& root_surface_id,
                                    const gfx::Point& point,
                                    gfx::Transform* transform,
                                    bool* out_query_renderer);

  // Returns whether the target surface falls inside the provide root surface.
  // Returns the |transform| to convert points from the root surface coordinate
  // space to the target surface coordinate space.
  bool GetTransformToTargetSurface(const SurfaceId& root_surface_id,
                                   const SurfaceId& target_surface_id,
                                   gfx::Transform* transform);

  // Attempts to transform a point from the coordinate space of one surface to
  // that of another, where one is surface is embedded within the other.
  // Returns true if the transform is successfully applied, and false if
  // neither surface is contained with the other.
  bool TransformPointToTargetSurface(const SurfaceId& original_surface_id,
                                     const SurfaceId& target_surface_id,
                                     gfx::PointF* point);

 private:
  bool GetTargetSurfaceAtPointInternal(
      const SurfaceId& surface_id,
      RenderPassId render_pass_id,
      const gfx::Point& point_in_root_target,
      std::set<const RenderPass*>* referenced_passes,
      SurfaceId* out_surface_id,
      gfx::Transform* out_transform,
      bool* out_query_renderer);

  bool GetTransformToTargetSurfaceInternal(
      const SurfaceId& root_surface_id,
      const SurfaceId& target_surface_id,
      RenderPassId render_pass_id,
      std::set<const RenderPass*>* referenced_passes,
      gfx::Transform* out_transform);

  const RenderPass* GetRenderPassForSurfaceById(const SurfaceId& surface_id,
                                                RenderPassId render_pass_id);

  bool PointInQuad(const DrawQuad* quad,
                   const gfx::Point& point_in_render_pass_space,
                   gfx::Transform* target_to_quad_transform,
                   gfx::Point* point_in_quad_space);

  SurfaceHittestDelegate* const delegate_;
  SurfaceManager* const manager_;
};
}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_HITTEST_H_
