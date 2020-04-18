// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PAGE_LOAD_METRICS_METRICS_RENDER_FRAME_OBSERVER_H_
#define CHROME_RENDERER_PAGE_LOAD_METRICS_METRICS_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "base/macros.h"
#include "chrome/common/page_load_metrics/page_load_timing.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/platform/web_loading_behavior_flag.h"

namespace base {
class Timer;
}  // namespace base

namespace page_load_metrics {

class PageTimingMetricsSender;
class PageTimingSender;

// MetricsRenderFrameObserver observes RenderFrame notifications, and sends page
// load timing information to the browser process over IPC. A
// MetricsRenderFrameObserver is instantiated for each frame (main frames and
// child frames). MetricsRenderFrameObserver dispatches timing and metadata
// updates for main frames, but only metadata updates for child frames.
class MetricsRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit MetricsRenderFrameObserver(content::RenderFrame* render_frame);
  ~MetricsRenderFrameObserver() override;

  // RenderFrameObserver implementation
  void DidChangePerformanceTiming() override;
  void DidObserveLoadingBehavior(
      blink::WebLoadingBehaviorFlag behavior) override;
  void DidObserveNewFeatureUsage(blink::mojom::WebFeature feature) override;
  void DidObserveNewCssPropertyUsage(int css_property,
                                     bool is_animated) override;
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_document_navigation) override;
  void OnDestruct() override;

  // Invoked when a frame is going away. This is our last chance to send IPCs
  // before being destroyed.
  void FrameDetached() override;

 private:
  void SendMetrics();
  virtual mojom::PageLoadTimingPtr GetTiming() const;
  virtual std::unique_ptr<base::Timer> CreateTimer();
  virtual std::unique_ptr<PageTimingSender> CreatePageTimingSender();
  virtual bool HasNoRenderFrame() const;

  // Will be null when we're not actively sending metrics.
  std::unique_ptr<PageTimingMetricsSender> page_timing_metrics_sender_;

  DISALLOW_COPY_AND_ASSIGN(MetricsRenderFrameObserver);
};

}  // namespace page_load_metrics

#endif  // CHROME_RENDERER_PAGE_LOAD_METRICS_METRICS_RENDER_FRAME_OBSERVER_H_
