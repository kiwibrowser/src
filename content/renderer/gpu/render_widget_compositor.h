// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_GPU_RENDER_WIDGET_COMPOSITOR_H_
#define CONTENT_RENDERER_GPU_RENDER_WIDGET_COMPOSITOR_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "cc/input/browser_controls_state.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/layer_tree_host_client.h"
#include "cc/trees/layer_tree_host_single_thread_client.h"
#include "cc/trees/layer_tree_settings.h"
#include "cc/trees/managed_memory_policy.h"
#include "cc/trees/swap_promise.h"
#include "cc/trees/swap_promise_monitor.h"
#include "content/common/content_export.h"
#include "content/common/render_frame_metadata.mojom.h"
#include "content/renderer/gpu/compositor_dependencies.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "third_party/blink/public/platform/web_layer_tree_view.h"
#include "ui/gfx/geometry/rect.h"

namespace base {
class CommandLine;
}

namespace cc {

class AnimationHost;
class InputHandler;
class Layer;
class LayerTreeFrameSink;
class LayerTreeHost;
class MutatorHost;
}

namespace gfx {
class ColorSpace;
}

namespace ui {
class LatencyInfo;
}

namespace content {
class RenderWidgetCompositorDelegate;
struct ScreenInfo;

class CONTENT_EXPORT RenderWidgetCompositor
    : public blink::WebLayerTreeView,
      public cc::LayerTreeHostClient,
      public cc::LayerTreeHostSingleThreadClient {
 public:
  // Attempt to construct and initialize a compositor instance for the widget
  // with the given settings. Returns NULL if initialization fails.
  static std::unique_ptr<RenderWidgetCompositor> Create(
      RenderWidgetCompositorDelegate* delegate,
      CompositorDependencies* compositor_deps);

  ~RenderWidgetCompositor() override;

  static cc::LayerTreeSettings GenerateLayerTreeSettings(
      const base::CommandLine& cmd,
      CompositorDependencies* compositor_deps,
      bool is_for_subframe,
      const ScreenInfo& screen_info,
      bool is_threaded);
  static std::unique_ptr<cc::LayerTreeHost> CreateLayerTreeHost(
      cc::LayerTreeHostClient* client,
      cc::LayerTreeHostSingleThreadClient* single_thread_client,
      cc::MutatorHost* mutator_host,
      CompositorDependencies* deps,
      const ScreenInfo& screen_info);

  void Initialize(std::unique_ptr<cc::LayerTreeHost> layer_tree_host,
                  std::unique_ptr<cc::AnimationHost> animation_host);

  static cc::ManagedMemoryPolicy GetGpuMemoryPolicy(
      const cc::ManagedMemoryPolicy& policy,
      const ScreenInfo& screen_info);

  void SetNeverVisible();
  const base::WeakPtr<cc::InputHandler>& GetInputHandler();
  void SetNeedsDisplayOnAllLayers();
  void SetRasterizeOnlyVisibleContent();
  void SetNeedsRedrawRect(gfx::Rect damage_rect);

  bool IsSurfaceSynchronizationEnabled() const;

  // Like setNeedsRedraw but forces the frame to be drawn, without early-outs.
  // Redraw will be forced after the next commit
  void SetNeedsForcedRedraw();
  // Calling CreateLatencyInfoSwapPromiseMonitor() to get a scoped
  // LatencyInfoSwapPromiseMonitor. During the life time of the
  // LatencyInfoSwapPromiseMonitor, if SetNeedsCommit() or
  // SetNeedsUpdateLayers() is called on LayerTreeHost, the original latency
  // info will be turned into a LatencyInfoSwapPromise.
  std::unique_ptr<cc::SwapPromiseMonitor> CreateLatencyInfoSwapPromiseMonitor(
      ui::LatencyInfo* latency);
  // Calling QueueSwapPromise() to directly queue a SwapPromise into
  // LayerTreeHost.
  void QueueSwapPromise(std::unique_ptr<cc::SwapPromise> swap_promise);
  int GetSourceFrameNumber() const;
  void NotifyInputThrottledUntilCommit();
  const cc::Layer* GetRootLayer() const;
  int ScheduleMicroBenchmark(
      const std::string& name,
      std::unique_ptr<base::Value> value,
      const base::Callback<void(std::unique_ptr<base::Value>)>& callback);
  bool SendMessageToMicroBenchmark(int id, std::unique_ptr<base::Value> value);
  void SetFrameSinkId(const viz::FrameSinkId& frame_sink_id);
  void SetRasterColorSpace(const gfx::ColorSpace& color_space);
  void SetIsForOopif(bool is_for_oopif);
  void SetContentSourceId(uint32_t source_id);
  void SetViewportSizeAndScale(const gfx::Size& device_viewport_size,
                               float device_scale_factor,
                               const viz::LocalSurfaceId& local_surface_id);
  void RequestNewLocalSurfaceId();
  bool HasNewLocalSurfaceIdRequest() const;
  void SetViewportVisibleRect(const gfx::Rect& visible_rect);
  void SetURLForUkm(const GURL& url);

  // WebLayerTreeView implementation.
  viz::FrameSinkId GetFrameSinkId() override;
  void SetRootLayer(scoped_refptr<cc::Layer> layer) override;
  void ClearRootLayer() override;
  cc::AnimationHost* CompositorAnimationHost() override;
  blink::WebSize GetViewportSize() const override;
  virtual blink::WebFloatPoint adjustEventPointForPinchZoom(
      const blink::WebFloatPoint& point) const;
  void SetBackgroundColor(SkColor color) override;
  void SetVisible(bool visible) override;
  void SetPageScaleFactorAndLimits(float page_scale_factor,
                                   float minimum,
                                   float maximum) override;
  void StartPageScaleAnimation(const blink::WebPoint& destination,
                               bool use_anchor,
                               float new_page_scale,
                               double duration_sec) override;
  bool HasPendingPageScaleAnimation() const override;
  void HeuristicsForGpuRasterizationUpdated(bool matches_heuristics) override;
  void SetNeedsBeginFrame() override;
  void DidStopFlinging() override;
  void LayoutAndPaintAsync(base::OnceClosure callback) override;
  void CompositeAndReadbackAsync(
      base::OnceCallback<void(const SkBitmap&)> callback) override;
  void SynchronouslyCompositeNoRasterForTesting() override;
  void CompositeWithRasterForTesting() override;
  void SetDeferCommits(bool defer_commits) override;
  void RegisterViewportLayers(
      const blink::WebLayerTreeView::ViewportLayers& viewport_layers) override;
  void ClearViewportLayers() override;
  void RegisterSelection(const blink::WebSelection& selection) override;
  void ClearSelection() override;
  void SetMutatorClient(std::unique_ptr<cc::LayerTreeMutator>) override;
  void ForceRecalculateRasterScales() override;
  void SetEventListenerProperties(
      blink::WebEventListenerClass eventClass,
      blink::WebEventListenerProperties properties) override;
  blink::WebEventListenerProperties EventListenerProperties(
      blink::WebEventListenerClass eventClass) const override;
  void SetHaveScrollEventHandlers(bool) override;
  bool HaveScrollEventHandlers() const override;
  int LayerTreeId() const override;
  void SetShowFPSCounter(bool show) override;
  void SetShowPaintRects(bool show) override;
  void SetShowDebugBorders(bool show) override;
  void SetShowScrollBottleneckRects(bool show) override;
  void NotifySwapTime(ReportTimeCallback callback) override;

  void UpdateBrowserControlsState(blink::WebBrowserControlsState constraints,
                                  blink::WebBrowserControlsState current,
                                  bool animate) override;
  void SetBrowserControlsHeight(float top_height,
                                float bottom_height,
                                bool shrink) override;
  void SetBrowserControlsShownRatio(float) override;
  void RequestDecode(const cc::PaintImage& image,
                     base::OnceCallback<void(bool)> callback) override;

  void SetOverscrollBehavior(const cc::OverscrollBehavior&) override;

  // cc::LayerTreeHostClient implementation.
  void WillBeginMainFrame() override;
  void DidBeginMainFrame() override;
  void BeginMainFrame(const viz::BeginFrameArgs& args) override;
  void BeginMainFrameNotExpectedSoon() override;
  void BeginMainFrameNotExpectedUntil(base::TimeTicks time) override;
  void UpdateLayerTreeHost(VisualStateUpdate requested_update) override;
  void ApplyViewportDeltas(const gfx::Vector2dF& inner_delta,
                           const gfx::Vector2dF& outer_delta,
                           const gfx::Vector2dF& elastic_overscroll_delta,
                           float page_scale,
                           float top_controls_delta) override;
  void RecordWheelAndTouchScrollingCount(bool has_scrolled_by_wheel,
                                         bool has_scrolled_by_touch) override;
  void RequestNewLayerTreeFrameSink() override;
  void DidInitializeLayerTreeFrameSink() override;
  void DidFailToInitializeLayerTreeFrameSink() override;
  void WillCommit() override;
  void DidCommit() override;
  void DidCommitAndDrawFrame() override;
  void DidReceiveCompositorFrameAck() override;
  void DidCompletePageScaleAnimation() override;
  bool IsForSubframe() override;

  // cc::LayerTreeHostSingleThreadClient implementation.
  void RequestScheduleAnimation() override;
  void DidSubmitCompositorFrame() override;
  void DidLoseLayerTreeFrameSink() override;
  void RequestBeginMainFrameNotExpected(bool new_state) override;

  const cc::LayerTreeSettings& GetLayerTreeSettings() const {
    return layer_tree_host_->GetSettings();
  }

  // Creates a cc::RenderFrameMetadataObserver, which is sent to the compositor
  // thread for binding.
  void CreateRenderFrameObserver(
      mojom::RenderFrameMetadataObserverRequest request,
      mojom::RenderFrameMetadataObserverClientPtrInfo client_info);

 protected:
  friend class RenderViewImplScaleFactorTest;

  RenderWidgetCompositor(RenderWidgetCompositorDelegate* delegate,
                         CompositorDependencies* compositor_deps);

  cc::LayerTreeHost* layer_tree_host() { return layer_tree_host_.get(); }

 private:
  void SetLayerTreeFrameSink(
      std::unique_ptr<cc::LayerTreeFrameSink> layer_tree_frame_sink);
  void InvokeLayoutAndPaintCallback();
  bool CompositeIsSynchronous() const;
  void SynchronouslyComposite(bool raster,
                              std::unique_ptr<cc::SwapPromise> swap_promise);

  RenderWidgetCompositorDelegate* const delegate_;
  CompositorDependencies* const compositor_deps_;
  const bool threaded_;
  std::unique_ptr<cc::AnimationHost> animation_host_;
  std::unique_ptr<cc::LayerTreeHost> layer_tree_host_;
  bool never_visible_;
  bool is_for_oopif_;

  bool layer_tree_frame_sink_request_failed_while_invisible_ = false;

  bool in_synchronous_compositor_update_ = false;
  base::OnceClosure layout_and_paint_async_callback_;

  viz::FrameSinkId frame_sink_id_;

  base::WeakPtrFactory<RenderWidgetCompositor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetCompositor);
};

}  // namespace content

#endif  // CONTENT_RENDERER_GPU_RENDER_WIDGET_COMPOSITOR_H_
