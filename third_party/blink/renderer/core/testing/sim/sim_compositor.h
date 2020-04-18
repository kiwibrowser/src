// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_SIM_SIM_COMPOSITOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_SIM_SIM_COMPOSITOR_H_

#include "base/time/time.h"
#include "third_party/blink/public/platform/web_layer_tree_view.h"
#include "third_party/blink/renderer/core/testing/sim/sim_canvas.h"

namespace blink {

class WebViewImpl;

// Simulated very basic compositor that's capable of running the BeginMainFrame
// processing steps on WebView: beginFrame, layout, paint.
//
// The painting capabilities are very limited in that only the main layer of
// every CompositedLayerMapping will be painted, squashed layers
// are not supported and the entirety of every layer is always repainted even if
// only part of the layer was invalid.
//
// Note: This also does not support compositor driven animations.
class SimCompositor final : public WebLayerTreeView {
 public:
  explicit SimCompositor();
  ~SimCompositor() override;

  void SetWebView(WebViewImpl&);

  // Executes the BeginMainFrame processing steps, an approximation of what
  // cc::ThreadProxy::BeginMainFrame would do.
  // If time is not specified a 60Hz frame rate time progression is used.
  // Returns all drawing commands that were issued during painting the frame
  // (including cached ones).
  // TODO(dcheng): This should take a base::TimeDelta.
  SimCanvas::Commands BeginFrame(double time_delta_in_seconds = 0.016);

  // Similar to BeginFrame() but doesn't require NeedsBeginFrame(). This is
  // useful for testing the painting after a frame is throttled (for which
  // we don't schedule a BeginFrame).
  SimCanvas::Commands PaintFrame();

  bool NeedsBeginFrame() const { return needs_begin_frame_; }
  bool DeferCommits() const { return defer_commits_; }

  bool HasSelection() const { return has_selection_; }

  void SetBackgroundColor(SkColor background_color) override {
    background_color_ = background_color;
  }

  SkColor background_color() { return background_color_; }

 private:
  void SetNeedsBeginFrame() override;
  void SetDeferCommits(bool) override;
  void RegisterSelection(const WebSelection&) override;
  void ClearSelection() override;

  bool needs_begin_frame_;
  bool defer_commits_;
  bool has_selection_;
  WebViewImpl* web_view_;
  base::TimeTicks last_frame_time_;
  SkColor background_color_;
};

}  // namespace blink

#endif
