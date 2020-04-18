// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_HEADS_UP_DISPLAY_LAYER_IMPL_H_
#define CC_LAYERS_HEADS_UP_DISPLAY_LAYER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "cc/cc_export.h"
#include "cc/layers/layer_impl.h"
#include "cc/resources/memory_history.h"
#include "cc/resources/resource_pool.h"
#include "cc/trees/debug_rect_history.h"
#include "third_party/skia/include/core/SkRefCnt.h"

class SkCanvas;
class SkPaint;
class SkTypeface;
struct SkRect;

namespace cc {
class FrameRateCounter;
class LayerTreeFrameSink;
class LayerTreeResourceProvider;

class CC_EXPORT HeadsUpDisplayLayerImpl : public LayerImpl {
 public:
  static std::unique_ptr<HeadsUpDisplayLayerImpl> Create(
      LayerTreeImpl* tree_impl,
      int id) {
    return base::WrapUnique(new HeadsUpDisplayLayerImpl(tree_impl, id));
  }
  ~HeadsUpDisplayLayerImpl() override;

  std::unique_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl) override;

  bool WillDraw(DrawMode draw_mode,
                LayerTreeResourceProvider* resource_provider) override;
  void AppendQuads(viz::RenderPass* render_pass,
                   AppendQuadsData* append_quads_data) override;
  void UpdateHudTexture(DrawMode draw_mode,
                        LayerTreeFrameSink* frame_sink,
                        LayerTreeResourceProvider* resource_provider,
                        bool gpu_raster,
                        const viz::RenderPassList& list);

  void ReleaseResources() override;

  gfx::Rect GetEnclosingRectInTargetSpace() const override;

  bool IsAnimatingHUDContents() const { return fade_step_ > 0; }

  void SetHUDTypeface(sk_sp<SkTypeface> typeface);

  // This evicts hud quad appended during render pass preparation.
  void EvictHudQuad(const viz::RenderPassList& list);

  // LayerImpl overrides.
  void PushPropertiesTo(LayerImpl* layer) override;

 private:
  class Graph {
   public:
    Graph(double indicator_value, double start_upper_bound);

    // Eases the upper bound, which limits what is currently visible in the
    // graph, so that the graph always scales to either it's max or
    // default_upper_bound.
    double UpdateUpperBound();

    double value;
    double min;
    double max;

    double current_upper_bound;
    const double default_upper_bound;
    const double indicator;
  };

  HeadsUpDisplayLayerImpl(LayerTreeImpl* tree_impl, int id);

  const char* LayerTypeAsString() const override;

  void AsValueInto(base::trace_event::TracedValue* dict) const override;

  void UpdateHudContents();
  void DrawHudContents(SkCanvas* canvas);

  int MeasureText(SkPaint* paint, const std::string& text, int size) const;
  void DrawText(SkCanvas* canvas,
                SkPaint* paint,
                const std::string& text,
                SkPaint::Align align,
                int size,
                int x,
                int y) const;
  void DrawText(SkCanvas* canvas,
                SkPaint* paint,
                const std::string& text,
                SkPaint::Align align,
                int size,
                const SkPoint& pos) const;
  void DrawGraphBackground(SkCanvas* canvas,
                           SkPaint* paint,
                           const SkRect& bounds) const;
  void DrawGraphLines(SkCanvas* canvas,
                      SkPaint* paint,
                      const SkRect& bounds,
                      const Graph& graph) const;

  SkRect DrawFPSDisplay(SkCanvas* canvas,
                        const FrameRateCounter* fps_counter,
                        int right,
                        int top) const;
  SkRect DrawMemoryDisplay(SkCanvas* canvas,
                           int top,
                           int right,
                           int width) const;
  SkRect DrawGpuRasterizationStatus(SkCanvas* canvas,
                                    int right,
                                    int top,
                                    int width) const;
  void DrawDebugRect(SkCanvas* canvas,
                     SkPaint* paint,
                     const DebugRect& rect,
                     SkColor stroke_color,
                     SkColor fill_color,
                     float stroke_width,
                     const std::string& label_text) const;
  void DrawDebugRects(SkCanvas* canvas, DebugRectHistory* debug_rect_history);

  ResourcePool::InUsePoolResource in_flight_resource_;
  std::unique_ptr<ResourcePool> pool_;
  viz::DrawQuad* current_quad_ = nullptr;
  // Used for software raster when it will be uploaded to a texture.
  sk_sp<SkSurface> staging_surface_;

  sk_sp<SkTypeface> typeface_;

  float internal_contents_scale_;
  gfx::Size internal_content_bounds_;

  Graph fps_graph_;
  Graph paint_time_graph_;
  MemoryHistory::Entry memory_entry_;
  int fade_step_;
  std::vector<DebugRect> paint_rects_;

  base::TimeTicks time_of_last_graph_update_;

  DISALLOW_COPY_AND_ASSIGN(HeadsUpDisplayLayerImpl);
};

}  // namespace cc

#endif  // CC_LAYERS_HEADS_UP_DISPLAY_LAYER_IMPL_H_
