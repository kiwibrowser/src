// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/latency_info_swap_promise_monitor.h"

#include <stdint.h>

#include "base/threading/platform_thread.h"
#include "cc/trees/latency_info_swap_promise.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "cc/trees/layer_tree_impl.h"
#include "cc/trees/swap_promise_manager.h"

namespace {

bool AddRenderingScheduledComponent(ui::LatencyInfo* latency_info,
                                    bool on_main) {
  ui::LatencyComponentType type =
      on_main ? ui::INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_MAIN_COMPONENT
              : ui::INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_IMPL_COMPONENT;
  if (latency_info->FindLatency(type, 0, nullptr))
    return false;
  latency_info->AddLatencyNumber(type, 0);
  return true;
}

bool AddForwardingScrollUpdateToMainComponent(ui::LatencyInfo* latency_info) {
  if (latency_info->FindLatency(
          ui::INPUT_EVENT_LATENCY_FORWARD_SCROLL_UPDATE_TO_MAIN_COMPONENT, 0,
          nullptr))
    return false;
  latency_info->AddLatencyNumber(
      ui::INPUT_EVENT_LATENCY_FORWARD_SCROLL_UPDATE_TO_MAIN_COMPONENT, 0);
  return true;
}

}  // namespace

namespace cc {

LatencyInfoSwapPromiseMonitor::LatencyInfoSwapPromiseMonitor(
    ui::LatencyInfo* latency,
    SwapPromiseManager* swap_promise_manager,
    LayerTreeHostImpl* host_impl)
    : SwapPromiseMonitor(swap_promise_manager, host_impl), latency_(latency) {}

LatencyInfoSwapPromiseMonitor::~LatencyInfoSwapPromiseMonitor() = default;

void LatencyInfoSwapPromiseMonitor::OnSetNeedsCommitOnMain() {
  if (AddRenderingScheduledComponent(latency_, true /* on_main */)) {
    std::unique_ptr<SwapPromise> swap_promise(
        new LatencyInfoSwapPromise(*latency_));
    swap_promise_manager_->QueueSwapPromise(std::move(swap_promise));
  }
}

void LatencyInfoSwapPromiseMonitor::OnSetNeedsRedrawOnImpl() {
  if (AddRenderingScheduledComponent(latency_, false /* on_main */)) {
    std::unique_ptr<SwapPromise> swap_promise(
        new LatencyInfoSwapPromise(*latency_));
    // Queue a pinned swap promise on the active tree. This will allow
    // measurement of the time to the next SwapBuffers(). The swap
    // promise is pinned so that it is not interrupted by new incoming
    // activations (which would otherwise break the swap promise).
    host_impl_->active_tree()->QueuePinnedSwapPromise(std::move(swap_promise));
  }
}

void LatencyInfoSwapPromiseMonitor::OnForwardScrollUpdateToMainThreadOnImpl() {
  if (AddForwardingScrollUpdateToMainComponent(latency_)) {
    ui::LatencyInfo new_latency;
    new_latency.CopyLatencyFrom(
        *latency_,
        ui::INPUT_EVENT_LATENCY_FORWARD_SCROLL_UPDATE_TO_MAIN_COMPONENT);
    new_latency.AddLatencyNumberWithTraceName(
        ui::LATENCY_BEGIN_SCROLL_LISTENER_UPDATE_MAIN_COMPONENT, 0,
        "ScrollUpdate");
    std::unique_ptr<SwapPromise> swap_promise(
        new LatencyInfoSwapPromise(new_latency));
    host_impl_->QueueSwapPromiseForMainThreadScrollUpdate(
        std::move(swap_promise));
  }
}

}  // namespace cc
