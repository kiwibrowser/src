// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_TARGETER_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_TARGETER_H_

#include <queue>
#include <unordered_set>

#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "content/common/content_constants_internal.h"
#include "content/common/content_export.h"
#include "ui/events/blink/web_input_event_traits.h"
#include "ui/latency/latency_info.h"

namespace blink {
class WebInputEvent;
}  // namespace blink

namespace gfx {
class PointF;
}

namespace viz {
class FrameSinkId;
}

namespace content {

class RenderWidgetHostViewBase;
class OneShotTimeoutMonitor;

struct CONTENT_EXPORT RenderWidgetTargetResult {
  RenderWidgetTargetResult();
  RenderWidgetTargetResult(const RenderWidgetTargetResult&);
  RenderWidgetTargetResult(RenderWidgetHostViewBase* view,
                           bool should_query_view,
                           base::Optional<gfx::PointF> location,
                           bool latched_target);
  ~RenderWidgetTargetResult();

  RenderWidgetHostViewBase* view = nullptr;
  bool should_query_view = false;
  base::Optional<gfx::PointF> target_location = base::nullopt;
  // When |latched_target| is false, we explicitly hit-tested events instead of
  // using a known target.
  bool latched_target = false;
};

class TracingUmaTracker;

class RenderWidgetTargeter {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    virtual RenderWidgetTargetResult FindTargetSynchronously(
        RenderWidgetHostViewBase* root_view,
        const blink::WebInputEvent& event) = 0;

    // |event| is in |root_view|'s coordinate space.
    virtual void DispatchEventToTarget(
        RenderWidgetHostViewBase* root_view,
        RenderWidgetHostViewBase* target,
        const blink::WebInputEvent& event,
        const ui::LatencyInfo& latency,
        const base::Optional<gfx::PointF>& target_location) = 0;

    virtual RenderWidgetHostViewBase* FindViewFromFrameSinkId(
        const viz::FrameSinkId& frame_sink_id) const = 0;
  };

  // The delegate must outlive this targeter.
  explicit RenderWidgetTargeter(Delegate* delegate);
  ~RenderWidgetTargeter();

  // Finds the appropriate target inside |root_view| for |event|, and dispatches
  // it through the delegate. |event| is in the coord-space of |root_view|.
  void FindTargetAndDispatch(RenderWidgetHostViewBase* root_view,
                             const blink::WebInputEvent& event,
                             const ui::LatencyInfo& latency);

  void ViewWillBeDestroyed(RenderWidgetHostViewBase* view);

  void set_async_hit_test_timeout_delay_for_testing(
      const base::TimeDelta& delay) {
    async_hit_test_timeout_delay_ = delay;
  }

  unsigned num_requests_in_queue_for_testing() { return requests_.size(); }
  bool is_request_in_flight_for_testing() { return request_in_flight_; }

 private:
  // Attempts to target and dispatch all events in the queue. It stops if it has
  // to query a client, or if the queue becomes empty.
  void FlushEventQueue();

  // Queries |target| to find the correct target for |event|.
  // |event| is in the coordinate space of |root_view|.
  // |target_location|, if set, is the location in |target|'s coordinate space.
  // |last_request_target| and |last_target_location| provide a fallback target
  // the case that the query times out. These should be null values when
  // querying the root view, and the target's immediate parent view otherwise.
  void QueryClient(RenderWidgetHostViewBase* root_view,
                   RenderWidgetHostViewBase* target,
                   const blink::WebInputEvent& event,
                   const ui::LatencyInfo& latency,
                   const gfx::PointF& target_location,
                   RenderWidgetHostViewBase* last_request_target,
                   const gfx::PointF& last_target_location);

  // |event| is in the coordinate space of |root_view|. |target_location|, if
  // set, is the location in |target|'s coordinate space.
  void FoundFrameSinkId(base::WeakPtr<RenderWidgetHostViewBase> root_view,
                        base::WeakPtr<RenderWidgetHostViewBase> target,
                        ui::WebScopedInputEvent event,
                        const ui::LatencyInfo& latency,
                        uint32_t request_id,
                        const gfx::PointF& target_location,
                        TracingUmaTracker tracker,
                        const viz::FrameSinkId& frame_sink_id);

  // |event| is in the coordinate space of |root_view|. |target_location|, if
  // set, is the location in |target|'s coordinate space. If |latched_target| is
  // false, we explicitly did hit-testing for this event, instead of using a
  // known target.
  void FoundTarget(RenderWidgetHostViewBase* root_view,
                   RenderWidgetHostViewBase* target,
                   const blink::WebInputEvent& event,
                   const ui::LatencyInfo& latency,
                   const base::Optional<gfx::PointF>& target_location,
                   bool latched_target);

  // Callback when the hit testing timer fires, to resume event processing
  // without further waiting for a response to the last targeting request.
  void AsyncHitTestTimedOut(
      base::WeakPtr<RenderWidgetHostViewBase> current_request_root_view,
      base::WeakPtr<RenderWidgetHostViewBase> current_request_target,
      const gfx::PointF& current_target_location,
      base::WeakPtr<RenderWidgetHostViewBase> last_request_target,
      const gfx::PointF& last_target_location,
      ui::WebScopedInputEvent event,
      const ui::LatencyInfo& latency);

  base::TimeDelta async_hit_test_timeout_delay() {
    return async_hit_test_timeout_delay_;
  }

  struct TargetingRequest {
    TargetingRequest();
    TargetingRequest(TargetingRequest&& request);
    TargetingRequest& operator=(TargetingRequest&& other);
    ~TargetingRequest();

    base::WeakPtr<RenderWidgetHostViewBase> root_view;
    ui::WebScopedInputEvent event;
    ui::LatencyInfo latency;
    std::unique_ptr<TracingUmaTracker> tracker;
  };

  bool request_in_flight_ = false;
  uint32_t last_request_id_ = 0;
  std::queue<TargetingRequest> requests_;

  std::unordered_set<RenderWidgetHostViewBase*> unresponsive_views_;

  // This value keeps track of the number of clients we have asked in order to
  // do async hit-testing.
  uint32_t async_depth_ = 0;

  // This value limits how long to wait for a response from the renderer
  // process before giving up and resuming event processing.
  base::TimeDelta async_hit_test_timeout_delay_ =
      base::TimeDelta::FromMilliseconds(kAsyncHitTestTimeoutMs);

  std::unique_ptr<OneShotTimeoutMonitor> async_hit_test_timeout_;

  Delegate* const delegate_;
  base::WeakPtrFactory<RenderWidgetTargeter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetTargeter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_TARGETER_H_
