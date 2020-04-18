// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/widget_input_handler_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "content/common/input_messages.h"
#include "content/renderer/gpu/render_widget_compositor.h"
#include "content/renderer/ime_event_guard.h"
#include "content/renderer/input/widget_input_handler_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/render_widget.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_main_thread_scheduler.h"
#include "third_party/blink/public/platform/web_coalesced_input_event.h"
#include "third_party/blink/public/platform/web_keyboard_event.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "ui/events/base_event_utils.h"

#if defined(OS_ANDROID)
#include "content/public/common/content_client.h"
#include "content/renderer/android/synchronous_compositor_proxy_mojo.h"
#include "content/renderer/android/synchronous_compositor_registry.h"
#endif

namespace content {
namespace {
void CallCallback(mojom::WidgetInputHandler::DispatchEventCallback callback,
                  InputEventAckState ack_state,
                  const ui::LatencyInfo& latency_info,
                  std::unique_ptr<ui::DidOverscrollParams> overscroll_params,
                  base::Optional<cc::TouchAction> touch_action) {
  std::move(callback).Run(
      InputEventAckSource::MAIN_THREAD, latency_info, ack_state,
      overscroll_params
          ? base::Optional<ui::DidOverscrollParams>(*overscroll_params)
          : base::nullopt,
      touch_action);
}

InputEventAckState InputEventDispositionToAck(
    ui::InputHandlerProxy::EventDisposition disposition) {
  switch (disposition) {
    case ui::InputHandlerProxy::DID_HANDLE:
      return INPUT_EVENT_ACK_STATE_CONSUMED;
    case ui::InputHandlerProxy::DID_NOT_HANDLE:
      return INPUT_EVENT_ACK_STATE_NOT_CONSUMED;
    case ui::InputHandlerProxy::DID_NOT_HANDLE_NON_BLOCKING_DUE_TO_FLING:
      return INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING_DUE_TO_FLING;
    case ui::InputHandlerProxy::DROP_EVENT:
      return INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS;
    case ui::InputHandlerProxy::DID_HANDLE_NON_BLOCKING:
      return INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING;
    case ui::InputHandlerProxy::DID_HANDLE_SHOULD_BUBBLE:
      return INPUT_EVENT_ACK_STATE_CONSUMED_SHOULD_BUBBLE;
  }
  NOTREACHED();
  return INPUT_EVENT_ACK_STATE_UNKNOWN;
}

}  // namespace

#if defined(OS_ANDROID)
class SynchronousCompositorProxyRegistry
    : public SynchronousCompositorRegistry {
 public:
  explicit SynchronousCompositorProxyRegistry(
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner)
      : compositor_task_runner_(std::move(compositor_task_runner)) {}

  ~SynchronousCompositorProxyRegistry() override {
    // Ensure the proxy has already been release on the compositor thread
    // before destroying this object.
    DCHECK(!proxy_);
  }

  void CreateProxy(ui::SynchronousInputHandlerProxy* handler) {
    DCHECK(compositor_task_runner_->BelongsToCurrentThread());
    proxy_ = std::make_unique<SynchronousCompositorProxyMojo>(handler);
    proxy_->Init();

    if (sink_)
      proxy_->SetLayerTreeFrameSink(sink_);
  }

  SynchronousCompositorProxyMojo* proxy() { return proxy_.get(); }

  void RegisterLayerTreeFrameSink(
      int routing_id,
      SynchronousLayerTreeFrameSink* layer_tree_frame_sink) override {
    DCHECK(compositor_task_runner_->BelongsToCurrentThread());
    DCHECK_EQ(nullptr, sink_);
    sink_ = layer_tree_frame_sink;
    if (proxy_)
      proxy_->SetLayerTreeFrameSink(layer_tree_frame_sink);
  }

  void UnregisterLayerTreeFrameSink(
      int routing_id,
      SynchronousLayerTreeFrameSink* layer_tree_frame_sink) override {
    DCHECK(compositor_task_runner_->BelongsToCurrentThread());
    DCHECK_EQ(layer_tree_frame_sink, sink_);
    sink_ = nullptr;
  }

  void DestroyProxy() {
    DCHECK(compositor_task_runner_->BelongsToCurrentThread());
    proxy_.reset();
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;
  std::unique_ptr<SynchronousCompositorProxyMojo> proxy_;
  SynchronousLayerTreeFrameSink* sink_ = nullptr;
};

#endif

scoped_refptr<WidgetInputHandlerManager> WidgetInputHandlerManager::Create(
    base::WeakPtr<RenderWidget> render_widget,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
    blink::scheduler::WebMainThreadScheduler* main_thread_scheduler) {
  scoped_refptr<WidgetInputHandlerManager> manager =
      new WidgetInputHandlerManager(std::move(render_widget),
                                    std::move(compositor_task_runner),
                                    main_thread_scheduler);
  manager->Init();
  return manager;
}

WidgetInputHandlerManager::WidgetInputHandlerManager(
    base::WeakPtr<RenderWidget> render_widget,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
    blink::scheduler::WebMainThreadScheduler* main_thread_scheduler)
    : render_widget_(render_widget),
      main_thread_scheduler_(main_thread_scheduler),
      input_event_queue_(render_widget->GetInputEventQueue()),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      compositor_task_runner_(std::move(compositor_task_runner)) {
#if defined(OS_ANDROID)
  if (compositor_task_runner_) {
    synchronous_compositor_registry_ =
        std::make_unique<SynchronousCompositorProxyRegistry>(
            compositor_task_runner_);
  }
#endif
}

void WidgetInputHandlerManager::Init() {
  if (compositor_task_runner_) {
    bool sync_compositing = false;
#if defined(OS_ANDROID)
    sync_compositing = GetContentClient()->UsingSynchronousCompositing();
#endif

    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &WidgetInputHandlerManager::InitOnCompositorThread, this,
            render_widget_->compositor()->GetInputHandler(),
            render_widget_->compositor_deps()->IsScrollAnimatorEnabled(),
            sync_compositing));
  }
}

WidgetInputHandlerManager::~WidgetInputHandlerManager() = default;

void WidgetInputHandlerManager::AddAssociatedInterface(
    mojom::WidgetInputHandlerAssociatedRequest request,
    mojom::WidgetInputHandlerHostPtr host) {
  if (compositor_task_runner_) {
    associated_host_ =
        mojo::ThreadSafeInterfacePtr<mojom::WidgetInputHandlerHost>::Create(
            host.PassInterface(), compositor_task_runner_);
    // Mojo channel bound on compositor thread.
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&WidgetInputHandlerManager::BindAssociatedChannel, this,
                       std::move(request)));
  } else {
    associated_host_ =
        mojo::ThreadSafeInterfacePtr<mojom::WidgetInputHandlerHost>::Create(
            std::move(host));
    // Mojo channel bound on main thread.
    BindAssociatedChannel(std::move(request));
  }
}

void WidgetInputHandlerManager::AddInterface(
    mojom::WidgetInputHandlerRequest request,
    mojom::WidgetInputHandlerHostPtr host) {
  if (compositor_task_runner_) {
    host_ = mojo::ThreadSafeInterfacePtr<mojom::WidgetInputHandlerHost>::Create(
        host.PassInterface(), compositor_task_runner_);
    // Mojo channel bound on compositor thread.
    compositor_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&WidgetInputHandlerManager::BindChannel, this,
                                  std::move(request)));
  } else {
    host_ = mojo::ThreadSafeInterfacePtr<mojom::WidgetInputHandlerHost>::Create(
        std::move(host));
    // Mojo channel bound on main thread.
    BindChannel(std::move(request));
  }
}

void WidgetInputHandlerManager::WillShutdown() {
#if defined(OS_ANDROID)
  if (synchronous_compositor_registry_)
    synchronous_compositor_registry_->DestroyProxy();
#endif
  input_handler_proxy_.reset();
}

void WidgetInputHandlerManager::DispatchNonBlockingEventToMainThread(
    ui::WebScopedInputEvent event,
    const ui::LatencyInfo& latency_info) {
  DCHECK(input_event_queue_);
  input_event_queue_->HandleEvent(
      std::move(event), latency_info, DISPATCH_TYPE_NON_BLOCKING,
      INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING, HandledEventCallback());
}

std::unique_ptr<blink::WebGestureCurve>
WidgetInputHandlerManager::CreateFlingAnimationCurve(
    blink::WebGestureDevice device_source,
    const blink::WebFloatPoint& velocity,
    const blink::WebSize& cumulative_scroll) {
  return blink::Platform::Current()->CreateFlingAnimationCurve(
      device_source, velocity, cumulative_scroll);
}

void WidgetInputHandlerManager::DidOverscroll(
    const gfx::Vector2dF& accumulated_overscroll,
    const gfx::Vector2dF& latest_overscroll_delta,
    const gfx::Vector2dF& current_fling_velocity,
    const gfx::PointF& causal_event_viewport_point,
    const cc::OverscrollBehavior& overscroll_behavior) {
  mojom::WidgetInputHandlerHost* host = GetWidgetInputHandlerHost();
  if (!host)
    return;
  ui::DidOverscrollParams params;
  params.accumulated_overscroll = accumulated_overscroll;
  params.latest_overscroll_delta = latest_overscroll_delta;
  params.current_fling_velocity = current_fling_velocity;
  params.causal_event_viewport_point = causal_event_viewport_point;
  params.overscroll_behavior = overscroll_behavior;
  host->DidOverscroll(params);
}

void WidgetInputHandlerManager::DidStopFlinging() {
  mojom::WidgetInputHandlerHost* host = GetWidgetInputHandlerHost();
  if (!host)
    return;
  host->DidStopFlinging();
}

void WidgetInputHandlerManager::DidAnimateForInput() {
  main_thread_scheduler_->DidAnimateForInputOnCompositorThread();
}

void WidgetInputHandlerManager::DidStartScrollingViewport() {
  mojom::WidgetInputHandlerHost* host = GetWidgetInputHandlerHost();
  if (!host)
    return;
  host->DidStartScrollingViewport();
}

void WidgetInputHandlerManager::GenerateScrollBeginAndSendToMainThread(
    const blink::WebGestureEvent& update_event) {
  DCHECK_EQ(update_event.GetType(), blink::WebInputEvent::kGestureScrollUpdate);
  blink::WebGestureEvent scroll_begin(update_event);
  scroll_begin.SetType(blink::WebInputEvent::kGestureScrollBegin);
  scroll_begin.data.scroll_begin.inertial_phase =
      update_event.data.scroll_update.inertial_phase;
  scroll_begin.data.scroll_begin.delta_x_hint =
      update_event.data.scroll_update.delta_x;
  scroll_begin.data.scroll_begin.delta_y_hint =
      update_event.data.scroll_update.delta_y;
  scroll_begin.data.scroll_begin.delta_hint_units =
      update_event.data.scroll_update.delta_units;

  DispatchNonBlockingEventToMainThread(
      ui::WebInputEventTraits::Clone(scroll_begin), ui::LatencyInfo());
}

void WidgetInputHandlerManager::SetWhiteListedTouchAction(
    cc::TouchAction touch_action,
    uint32_t unique_touch_event_id,
    ui::InputHandlerProxy::EventDisposition event_disposition) {
  mojom::WidgetInputHandlerHost* host = GetWidgetInputHandlerHost();
  if (!host)
    return;
  InputEventAckState ack_state = InputEventDispositionToAck(event_disposition);
  host->SetWhiteListedTouchAction(touch_action, unique_touch_event_id,
                                  ack_state);
}

void WidgetInputHandlerManager::ProcessTouchAction(
    cc::TouchAction touch_action) {
  // Cancel the touch timeout on TouchActionNone since it is a good hint
  // that author doesn't want scrolling.
  if (touch_action == cc::TouchAction::kTouchActionNone) {
    if (mojom::WidgetInputHandlerHost* host = GetWidgetInputHandlerHost())
      host->CancelTouchTimeout();
  }
}

mojom::WidgetInputHandlerHost*
WidgetInputHandlerManager::GetWidgetInputHandlerHost() {
  if (associated_host_)
    return associated_host_.get()->get();
  if (host_)
    return host_.get()->get();
  return nullptr;
}

void WidgetInputHandlerManager::AttachSynchronousCompositor(
    mojom::SynchronousCompositorControlHostPtr control_host,
    mojom::SynchronousCompositorHostAssociatedPtrInfo host,
    mojom::SynchronousCompositorAssociatedRequest compositor_request) {
#if defined(OS_ANDROID)
  DCHECK(synchronous_compositor_registry_);
  synchronous_compositor_registry_->proxy()->BindChannel(
      std::move(control_host), std::move(host), std::move(compositor_request));
#endif
}

void WidgetInputHandlerManager::ObserveGestureEventOnMainThread(
    const blink::WebGestureEvent& gesture_event,
    const cc::InputHandlerScrollResult& scroll_result) {
  if (compositor_task_runner_) {
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &WidgetInputHandlerManager::ObserveGestureEventOnCompositorThread,
            this, gesture_event, scroll_result));
  }
}

void WidgetInputHandlerManager::DispatchEvent(
    std::unique_ptr<content::InputEvent> event,
    mojom::WidgetInputHandler::DispatchEventCallback callback) {
  if (!event || !event->web_event) {
    // Call |callback| if it was available indicating this event wasn't
    // handled.
    if (callback) {
      std::move(callback).Run(
          InputEventAckSource::MAIN_THREAD, ui::LatencyInfo(),
          INPUT_EVENT_ACK_STATE_NOT_CONSUMED, base::nullopt, base::nullopt);
    }
    return;
  }

  // If TimeTicks is not consistent across processes we cannot use the event's
  // platform timestamp in this process. Instead use the time that the event is
  // received as the event's timestamp.
  if (!base::TimeTicks::IsConsistentAcrossProcesses()) {
    event->web_event->SetTimeStamp(base::TimeTicks::Now());
  }

  if (compositor_task_runner_) {
    // If the input_handler_proxy has disappeared ensure we just ack event.
    if (!input_handler_proxy_) {
      if (callback) {
        std::move(callback).Run(
            InputEventAckSource::MAIN_THREAD, ui::LatencyInfo(),
            INPUT_EVENT_ACK_STATE_NOT_CONSUMED, base::nullopt, base::nullopt);
      }
      return;
    }
    CHECK(!main_thread_task_runner_->BelongsToCurrentThread());
    input_handler_proxy_->HandleInputEventWithLatencyInfo(
        std::move(event->web_event), event->latency_info,
        base::BindOnce(
            &WidgetInputHandlerManager::DidHandleInputEventAndOverscroll, this,
            std::move(callback)));
  } else {
    HandleInputEvent(std::move(event->web_event), event->latency_info,
                     std::move(callback));
  }
}

void WidgetInputHandlerManager::InitOnCompositorThread(
    const base::WeakPtr<cc::InputHandler>& input_handler,
    bool smooth_scroll_enabled,
    bool sync_compositing) {
  input_handler_proxy_ = std::make_unique<ui::InputHandlerProxy>(
      input_handler.get(), this,
      base::FeatureList::IsEnabled(features::kTouchpadAndWheelScrollLatching),
      base::FeatureList::IsEnabled(features::kAsyncWheelEvents));
  input_handler_proxy_->set_smooth_scroll_enabled(smooth_scroll_enabled);

#if defined(OS_ANDROID)
  if (sync_compositing) {
    DCHECK(synchronous_compositor_registry_);
    synchronous_compositor_registry_->CreateProxy(input_handler_proxy_.get());
  }
#endif
}

void WidgetInputHandlerManager::BindAssociatedChannel(
    mojom::WidgetInputHandlerAssociatedRequest request) {
  if (!request.is_pending())
    return;
  // Don't pass the |input_event_queue_| on if we don't have a
  // |compositor_task_runner_| as events might get out of order.
  WidgetInputHandlerImpl* handler = new WidgetInputHandlerImpl(
      this, main_thread_task_runner_,
      compositor_task_runner_ ? input_event_queue_ : nullptr, render_widget_);
  handler->SetAssociatedBinding(std::move(request));
}

void WidgetInputHandlerManager::BindChannel(
    mojom::WidgetInputHandlerRequest request) {
  if (!request.is_pending())
    return;
  // Don't pass the |input_event_queue_| on if we don't have a
  // |compositor_task_runner_| as events might get out of order.
  WidgetInputHandlerImpl* handler = new WidgetInputHandlerImpl(
      this, main_thread_task_runner_,
      compositor_task_runner_ ? input_event_queue_ : nullptr, render_widget_);
  handler->SetBinding(std::move(request));
}

void WidgetInputHandlerManager::HandleInputEvent(
    const ui::WebScopedInputEvent& event,
    const ui::LatencyInfo& latency,
    mojom::WidgetInputHandler::DispatchEventCallback callback) {
  if (!render_widget_ || render_widget_->is_swapped_out() ||
      render_widget_->IsClosing()) {
    if (callback) {
      std::move(callback).Run(InputEventAckSource::MAIN_THREAD, latency,
                              INPUT_EVENT_ACK_STATE_NOT_CONSUMED, base::nullopt,
                              base::nullopt);
    }
    return;
  }
  auto send_callback = base::BindOnce(
      &WidgetInputHandlerManager::HandledInputEvent, this, std::move(callback));

  blink::WebCoalescedInputEvent coalesced_event(*event);
  render_widget_->HandleInputEvent(coalesced_event, latency,
                                   std::move(send_callback));
}

void WidgetInputHandlerManager::DidHandleInputEventAndOverscroll(
    mojom::WidgetInputHandler::DispatchEventCallback callback,
    ui::InputHandlerProxy::EventDisposition event_disposition,
    ui::WebScopedInputEvent input_event,
    const ui::LatencyInfo& latency_info,
    std::unique_ptr<ui::DidOverscrollParams> overscroll_params) {
  InputEventAckState ack_state = InputEventDispositionToAck(event_disposition);
  if (ack_state == INPUT_EVENT_ACK_STATE_CONSUMED) {
    main_thread_scheduler_->DidHandleInputEventOnCompositorThread(
        *input_event, blink::scheduler::WebMainThreadScheduler::
                          InputEventState::EVENT_CONSUMED_BY_COMPOSITOR);
  } else if (MainThreadEventQueue::IsForwardedAndSchedulerKnown(ack_state)) {
    main_thread_scheduler_->DidHandleInputEventOnCompositorThread(
        *input_event, blink::scheduler::WebMainThreadScheduler::
                          InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD);
  }

  if (ack_state == INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING ||
      ack_state == INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING_DUE_TO_FLING ||
      ack_state == INPUT_EVENT_ACK_STATE_NOT_CONSUMED) {
    DCHECK(!overscroll_params);
    DCHECK(!latency_info.coalesced());
    InputEventDispatchType dispatch_type = callback.is_null()
                                               ? DISPATCH_TYPE_NON_BLOCKING
                                               : DISPATCH_TYPE_BLOCKING;
    HandledEventCallback handled_event =
        base::BindOnce(&WidgetInputHandlerManager::HandledInputEvent, this,
                       std::move(callback));
    input_event_queue_->HandleEvent(std::move(input_event), latency_info,
                                    dispatch_type, ack_state,
                                    std::move(handled_event));
    return;
  }
  if (callback) {
    std::move(callback).Run(
        InputEventAckSource::COMPOSITOR_THREAD, latency_info, ack_state,
        overscroll_params
            ? base::Optional<ui::DidOverscrollParams>(*overscroll_params)
            : base::nullopt,
        base::nullopt);
  }
}

void WidgetInputHandlerManager::HandledInputEvent(
    mojom::WidgetInputHandler::DispatchEventCallback callback,
    InputEventAckState ack_state,
    const ui::LatencyInfo& latency_info,
    std::unique_ptr<ui::DidOverscrollParams> overscroll_params,
    base::Optional<cc::TouchAction> touch_action) {
  if (!callback)
    return;

  // This method is called from either the main thread or the compositor thread.
  bool is_compositor_thread = compositor_task_runner_ &&
                              compositor_task_runner_->BelongsToCurrentThread();

  // If there is a compositor task runner and the current thread isn't the
  // compositor thread proxy it over to the compositor thread.
  if (compositor_task_runner_ && !is_compositor_thread) {
    compositor_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(CallCallback, std::move(callback), ack_state,
                                  latency_info, std::move(overscroll_params),
                                  touch_action));
  } else {
    // Otherwise call the callback immediately.
    std::move(callback).Run(
        is_compositor_thread ? InputEventAckSource::COMPOSITOR_THREAD
                             : InputEventAckSource::MAIN_THREAD,
        latency_info, ack_state,
        overscroll_params
            ? base::Optional<ui::DidOverscrollParams>(*overscroll_params)
            : base::nullopt,
        touch_action);
  }
}

void WidgetInputHandlerManager::ObserveGestureEventOnCompositorThread(
    const blink::WebGestureEvent& gesture_event,
    const cc::InputHandlerScrollResult& scroll_result) {
  if (!input_handler_proxy_)
    return;
  DCHECK(input_handler_proxy_->scroll_elasticity_controller());
  input_handler_proxy_->scroll_elasticity_controller()
      ->ObserveGestureEventAndResult(gesture_event, scroll_result);
}

#if defined(OS_ANDROID)
content::SynchronousCompositorRegistry*
WidgetInputHandlerManager::GetSynchronousCompositorRegistry() {
  DCHECK(synchronous_compositor_registry_);
  return synchronous_compositor_registry_.get();
}
#endif

}  // namespace content
