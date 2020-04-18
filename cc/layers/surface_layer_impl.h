// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_SURFACE_LAYER_IMPL_H_
#define CC_LAYERS_SURFACE_LAYER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "cc/cc_export.h"
#include "cc/layers/layer_impl.h"
#include "components/viz/common/quads/surface_draw_quad.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/surfaces/surface_info.h"

namespace cc {

class CC_EXPORT SurfaceLayerImpl : public LayerImpl {
 public:
  static std::unique_ptr<SurfaceLayerImpl> Create(LayerTreeImpl* tree_impl,
                                                  int id) {
    return base::WrapUnique(new SurfaceLayerImpl(tree_impl, id));
  }
  ~SurfaceLayerImpl() override;

  void SetPrimarySurfaceId(const viz::SurfaceId& surface_id,
                           base::Optional<uint32_t> deadline_in_frames);
  const viz::SurfaceId& primary_surface_id() const {
    return primary_surface_id_;
  }

  // A fallback Surface is a Surface that is already known to exist in the
  // display compositor. If surface synchronization is enabled, the display
  // compositor will use the fallback if the primary surface is unavailable
  // at the time of surface aggregation. If surface synchronization is not
  // enabled, then the primary and fallback surfaces will always match.
  void SetFallbackSurfaceId(const viz::SurfaceId& surface_id);
  const viz::SurfaceId& fallback_surface_id() const {
    return fallback_surface_id_;
  }

  base::Optional<uint32_t> deadline_in_frames() const {
    return deadline_in_frames_;
  }

  void SetStretchContentToFillBounds(bool stretch_content);
  bool stretch_content_to_fill_bounds() const {
    return stretch_content_to_fill_bounds_;
  }

  void SetSurfaceHitTestable(bool surface_hit_testable);
  bool surface_hit_testable() const { return surface_hit_testable_; }

  // LayerImpl overrides.
  std::unique_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl) override;
  void PushPropertiesTo(LayerImpl* layer) override;
  void AppendQuads(viz::RenderPass* render_pass,
                   AppendQuadsData* append_quads_data) override;
  bool is_surface_layer() const override;

 protected:
  SurfaceLayerImpl(LayerTreeImpl* tree_impl, int id);

 private:
  viz::SurfaceDrawQuad* CreateSurfaceDrawQuad(
      viz::RenderPass* render_pass,
      const viz::SurfaceId& surface_id,
      const base::Optional<viz::SurfaceId>& fallback_surface_id);

  void GetDebugBorderProperties(SkColor* color, float* width) const override;
  void AppendRainbowDebugBorder(viz::RenderPass* render_pass);
  void AsValueInto(base::trace_event::TracedValue* dict) const override;
  const char* LayerTypeAsString() const override;

  viz::SurfaceId primary_surface_id_;
  viz::SurfaceId fallback_surface_id_;
  base::Optional<uint32_t> deadline_in_frames_;

  bool stretch_content_to_fill_bounds_ = false;
  bool surface_hit_testable_ = false;

  DISALLOW_COPY_AND_ASSIGN(SurfaceLayerImpl);
};

}  // namespace cc

#endif  // CC_LAYERS_SURFACE_LAYER_IMPL_H_
