// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_EMULATION_AGENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_EMULATION_AGENT_H_

#include "base/macros.h"
#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/inspector/inspector_base_agent.h"
#include "third_party/blink/renderer/core/inspector/protocol/Emulation.h"
#include "third_party/blink/renderer/core/loader/frame_loader_types.h"
#include "third_party/blink/renderer/platform/scheduler/public/page_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class WebLocalFrameImpl;
class WebViewImpl;

namespace protocol {
namespace DOM {
class RGBA;
}  // namespace DOM
}  // namespace protocol

class CORE_EXPORT InspectorEmulationAgent final
    : public InspectorBaseAgent<protocol::Emulation::Metainfo>,
      public PageScheduler::VirtualTimeObserver {
 public:
  explicit InspectorEmulationAgent(WebLocalFrameImpl*);
  ~InspectorEmulationAgent() override;

  // protocol::Dispatcher::EmulationCommandHandler implementation.
  protocol::Response resetPageScaleFactor() override;
  protocol::Response setPageScaleFactor(double) override;
  protocol::Response setScriptExecutionDisabled(bool value) override;
  protocol::Response setTouchEmulationEnabled(
      bool enabled,
      protocol::Maybe<int> max_touch_points) override;
  protocol::Response setEmulatedMedia(const String&) override;
  protocol::Response setCPUThrottlingRate(double) override;
  protocol::Response setVirtualTimePolicy(
      const String& policy,
      protocol::Maybe<double> virtual_time_budget_ms,
      protocol::Maybe<int> max_virtual_time_task_starvation_count,
      protocol::Maybe<bool> wait_for_navigation,
      protocol::Maybe<double> initial_virtual_time,
      double* virtual_time_ticks_base_ms) override;
  protocol::Response setNavigatorOverrides(const String& platform) override;
  protocol::Response setDefaultBackgroundColorOverride(
      protocol::Maybe<protocol::DOM::RGBA>) override;
  protocol::Response setDeviceMetricsOverride(
      int width,
      int height,
      double device_scale_factor,
      bool mobile,
      protocol::Maybe<double> scale,
      protocol::Maybe<int> screen_width,
      protocol::Maybe<int> screen_height,
      protocol::Maybe<int> position_x,
      protocol::Maybe<int> position_y,
      protocol::Maybe<bool> dont_set_visible_size,
      protocol::Maybe<protocol::Emulation::ScreenOrientation>,
      protocol::Maybe<protocol::Page::Viewport>) override;
  protocol::Response clearDeviceMetricsOverride() override;

  // InspectorInstrumentation API
  void FrameStartedLoading(LocalFrame*, FrameLoadType);

  // InspectorBaseAgent overrides.
  protocol::Response disable() override;
  void Restore() override;

  // scheduler::PageScheduler::VirtualTimeObserver implementation.
  void OnVirtualTimeAdvanced(WTF::TimeDelta virtual_time_offset) override;
  void OnVirtualTimePaused(WTF::TimeDelta virtual_time_offset) override;

  void Trace(blink::Visitor*) override;

 private:
  WebViewImpl* GetWebViewImpl();
  void VirtualTimeBudgetExpired();

  struct PendingVirtualTimePolicy {
    PageScheduler::VirtualTimePolicy policy;
    base::Optional<double> virtual_time_budget_ms;
    base::Optional<int> max_virtual_time_task_starvation_count;
  };
  void ApplyVirtualTimePolicy(const PendingVirtualTimePolicy& new_policy);

  Member<WebLocalFrameImpl> web_local_frame_;
  bool virtual_time_setup_ = false;
  WTF::TimeTicks virtual_time_base_ticks_;

  // Supports a virtual time policy change scheduled to occur after any
  // navigation has started.
  base::Optional<PendingVirtualTimePolicy> pending_virtual_time_policy_;

  DISALLOW_COPY_AND_ASSIGN(InspectorEmulationAgent);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_EMULATION_AGENT_H_
