// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/testing/web_layer_tree_view_impl_for_testing.h"

#include "base/threading/thread_task_runner_handle.h"
#include "cc/animation/animation_host.h"
#include "cc/animation/animation_timeline.h"
#include "cc/layers/layer.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/layer_tree_settings.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_layer_tree_view.h"
#include "third_party/blink/public/platform/web_size.h"

namespace blink {

WebLayerTreeViewImplForTesting::WebLayerTreeViewImplForTesting()
    : WebLayerTreeViewImplForTesting(DefaultLayerTreeSettings()) {}

WebLayerTreeViewImplForTesting::WebLayerTreeViewImplForTesting(
    const cc::LayerTreeSettings& settings) {
  animation_host_ = cc::AnimationHost::CreateMainInstance();
  cc::LayerTreeHost::InitParams params;
  params.client = this;
  params.settings = &settings;
  params.main_task_runner = base::ThreadTaskRunnerHandle::Get();
  params.task_graph_runner = &task_graph_runner_;
  params.mutator_host = animation_host_.get();
  layer_tree_host_ = cc::LayerTreeHost::CreateSingleThreaded(this, &params);
  DCHECK(layer_tree_host_);
}

WebLayerTreeViewImplForTesting::~WebLayerTreeViewImplForTesting() = default;

// static
cc::LayerTreeSettings
WebLayerTreeViewImplForTesting::DefaultLayerTreeSettings() {
  cc::LayerTreeSettings settings;

  // For web contents, layer transforms should scale up the contents of layers
  // to keep content always crisp when possible.
  settings.layer_transforms_should_scale_layer_contents = true;

  return settings;
}

bool WebLayerTreeViewImplForTesting::HasLayer(const cc::Layer& layer) {
  return layer.GetLayerTreeHostForTesting() == layer_tree_host_.get();
}

void WebLayerTreeViewImplForTesting::SetViewportSize(
    const WebSize& device_viewport_size) {
  gfx::Size gfx_size(std::max(0, device_viewport_size.width),
                     std::max(0, device_viewport_size.height));
  // TODO(ccameron): This likely causes surface invariant violations.
  layer_tree_host_->SetViewportSizeAndScale(
      gfx_size, layer_tree_host_->device_scale_factor(),
      layer_tree_host_->local_surface_id_from_parent());
}

void WebLayerTreeViewImplForTesting::SetRootLayer(
    scoped_refptr<cc::Layer> root) {
  layer_tree_host_->SetRootLayer(root);
}

void WebLayerTreeViewImplForTesting::ClearRootLayer() {
  layer_tree_host_->SetRootLayer(scoped_refptr<cc::Layer>());
}

cc::AnimationHost* WebLayerTreeViewImplForTesting::CompositorAnimationHost() {
  return animation_host_.get();
}

WebSize WebLayerTreeViewImplForTesting::GetViewportSize() const {
  return WebSize(layer_tree_host_->device_viewport_size().width(),
                 layer_tree_host_->device_viewport_size().height());
}

void WebLayerTreeViewImplForTesting::SetBackgroundColor(SkColor color) {
  layer_tree_host_->set_background_color(color);
}

void WebLayerTreeViewImplForTesting::SetVisible(bool visible) {
  layer_tree_host_->SetVisible(visible);
}

void WebLayerTreeViewImplForTesting::SetPageScaleFactorAndLimits(
    float page_scale_factor,
    float minimum,
    float maximum) {
  layer_tree_host_->SetPageScaleFactorAndLimits(page_scale_factor, minimum,
                                                maximum);
}

void WebLayerTreeViewImplForTesting::StartPageScaleAnimation(
    const blink::WebPoint& scroll,
    bool use_anchor,
    float new_page_scale,
    double duration_sec) {}

void WebLayerTreeViewImplForTesting::SetNeedsBeginFrame() {
  layer_tree_host_->SetNeedsAnimate();
}

void WebLayerTreeViewImplForTesting::DidStopFlinging() {}

void WebLayerTreeViewImplForTesting::SetDeferCommits(bool defer_commits) {
  layer_tree_host_->SetDeferCommits(defer_commits);
}

void WebLayerTreeViewImplForTesting::UpdateLayerTreeHost(
    VisualStateUpdate requested_update) {}

void WebLayerTreeViewImplForTesting::ApplyViewportDeltas(
    const gfx::Vector2dF& inner_delta,
    const gfx::Vector2dF& outer_delta,
    const gfx::Vector2dF& elastic_overscroll_delta,
    float page_scale,
    float browser_controls_delta) {}

void WebLayerTreeViewImplForTesting::RecordWheelAndTouchScrollingCount(
    bool has_scrolled_by_wheel,
    bool has_scrolled_by_touch) {}

void WebLayerTreeViewImplForTesting::RequestNewLayerTreeFrameSink() {
  // Intentionally do not create and set a LayerTreeFrameSink.
}

void WebLayerTreeViewImplForTesting::DidFailToInitializeLayerTreeFrameSink() {
  NOTREACHED();
}

void WebLayerTreeViewImplForTesting::RegisterViewportLayers(
    const WebLayerTreeView::ViewportLayers& layers) {
  cc::LayerTreeHost::ViewportLayers viewport_layers;
  if (layers.overscroll_elasticity) {
    viewport_layers.overscroll_elasticity = layers.overscroll_elasticity;
  }
  viewport_layers.page_scale = layers.page_scale;
  if (layers.inner_viewport_container) {
    viewport_layers.inner_viewport_container = layers.inner_viewport_container;
  }
  if (layers.outer_viewport_container) {
    viewport_layers.outer_viewport_container = layers.outer_viewport_container;
  }
  viewport_layers.inner_viewport_scroll = layers.inner_viewport_scroll;
  if (layers.outer_viewport_scroll) {
    viewport_layers.outer_viewport_scroll = layers.outer_viewport_scroll;
  }
  layer_tree_host_->RegisterViewportLayers(viewport_layers);
}

void WebLayerTreeViewImplForTesting::ClearViewportLayers() {
  layer_tree_host_->RegisterViewportLayers(cc::LayerTreeHost::ViewportLayers());
}

void WebLayerTreeViewImplForTesting::RegisterSelection(
    const blink::WebSelection& selection) {}

void WebLayerTreeViewImplForTesting::ClearSelection() {}

void WebLayerTreeViewImplForTesting::SetEventListenerProperties(
    blink::WebEventListenerClass event_class,
    blink::WebEventListenerProperties properties) {
  // Equality of static_cast is checked in render_widget_compositor.cc.
  layer_tree_host_->SetEventListenerProperties(
      static_cast<cc::EventListenerClass>(event_class),
      static_cast<cc::EventListenerProperties>(properties));
}

blink::WebEventListenerProperties
WebLayerTreeViewImplForTesting::EventListenerProperties(
    blink::WebEventListenerClass event_class) const {
  // Equality of static_cast is checked in render_widget_compositor.cc.
  return static_cast<blink::WebEventListenerProperties>(
      layer_tree_host_->event_listener_properties(
          static_cast<cc::EventListenerClass>(event_class)));
}

void WebLayerTreeViewImplForTesting::SetHaveScrollEventHandlers(
    bool have_eent_handlers) {
  layer_tree_host_->SetHaveScrollEventHandlers(have_eent_handlers);
}

bool WebLayerTreeViewImplForTesting::HaveScrollEventHandlers() const {
  return layer_tree_host_->have_scroll_event_handlers();
}

bool WebLayerTreeViewImplForTesting::IsForSubframe() {
  return false;
}

}  // namespace blink
