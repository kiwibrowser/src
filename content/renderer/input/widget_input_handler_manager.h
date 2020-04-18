// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_WIDGET_INPUT_HANDLER_MANAGER_H_
#define CONTENT_RENDERER_INPUT_WIDGET_INPUT_HANDLER_MANAGER_H_

#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "content/common/content_export.h"
#include "content/common/input/input_handler.mojom.h"
#include "content/renderer/render_frame_impl.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/thread_safe_interface_ptr.h"
#include "ui/events/blink/input_handler_proxy.h"
#include "ui/events/blink/input_handler_proxy_client.h"

namespace blink {
namespace scheduler {
class WebMainThreadScheduler;
};  // namespace scheduler
};  // namespace blink

namespace content {
class MainThreadEventQueue;
class SynchronousCompositorRegistry;
class SynchronousCompositorProxyRegistry;

// This class maintains the compositor InputHandlerProxy and is
// responsible for passing input events on the compositor and main threads.
// The lifecycle of this object matches that of the RenderWidget.
class CONTENT_EXPORT WidgetInputHandlerManager
    : public base::RefCountedThreadSafe<WidgetInputHandlerManager>,
      public ui::InputHandlerProxyClient {
 public:
  static scoped_refptr<WidgetInputHandlerManager> Create(
      base::WeakPtr<RenderWidget> render_widget,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
      blink::scheduler::WebMainThreadScheduler* main_thread_scheduler);
  void AddAssociatedInterface(
      mojom::WidgetInputHandlerAssociatedRequest interface_request,
      mojom::WidgetInputHandlerHostPtr host);

  void AddInterface(mojom::WidgetInputHandlerRequest interface_request,
                    mojom::WidgetInputHandlerHostPtr host);

  // InputHandlerProxyClient overrides.
  void WillShutdown() override;
  void DispatchNonBlockingEventToMainThread(
      ui::WebScopedInputEvent event,
      const ui::LatencyInfo& latency_info) override;
  std::unique_ptr<blink::WebGestureCurve> CreateFlingAnimationCurve(
      blink::WebGestureDevice device_source,
      const blink::WebFloatPoint& velocity,
      const blink::WebSize& cumulative_scroll) override;

  void DidOverscroll(
      const gfx::Vector2dF& accumulated_overscroll,
      const gfx::Vector2dF& latest_overscroll_delta,
      const gfx::Vector2dF& current_fling_velocity,
      const gfx::PointF& causal_event_viewport_point,
      const cc::OverscrollBehavior& overscroll_behavior) override;
  void DidStopFlinging() override;
  void DidAnimateForInput() override;
  void DidStartScrollingViewport() override;
  void GenerateScrollBeginAndSendToMainThread(
      const blink::WebGestureEvent& update_event) override;
  void SetWhiteListedTouchAction(
      cc::TouchAction touch_action,
      uint32_t unique_touch_event_id,
      ui::InputHandlerProxy::EventDisposition event_disposition) override;

  void ObserveGestureEventOnMainThread(
      const blink::WebGestureEvent& gesture_event,
      const cc::InputHandlerScrollResult& scroll_result);

  void DispatchEvent(std::unique_ptr<content::InputEvent> event,
                     mojom::WidgetInputHandler::DispatchEventCallback callback);

  void ProcessTouchAction(cc::TouchAction touch_action);

  mojom::WidgetInputHandlerHost* GetWidgetInputHandlerHost();

  void AttachSynchronousCompositor(
      mojom::SynchronousCompositorControlHostPtr control_host,
      mojom::SynchronousCompositorHostAssociatedPtrInfo host,
      mojom::SynchronousCompositorAssociatedRequest compositor_request);

#if defined(OS_ANDROID)
  content::SynchronousCompositorRegistry* GetSynchronousCompositorRegistry();
#endif

 protected:
  friend class base::RefCountedThreadSafe<WidgetInputHandlerManager>;
  ~WidgetInputHandlerManager() override;

 private:
  WidgetInputHandlerManager(
      base::WeakPtr<RenderWidget> render_widget,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
      blink::scheduler::WebMainThreadScheduler* main_thread_scheduler);
  void Init();
  void InitOnCompositorThread(
      const base::WeakPtr<cc::InputHandler>& input_handler,
      bool smooth_scroll_enabled,
      bool sync_compositing);
  void BindAssociatedChannel(
      mojom::WidgetInputHandlerAssociatedRequest request);
  void BindChannel(mojom::WidgetInputHandlerRequest request);
  void HandleInputEvent(
      const ui::WebScopedInputEvent& event,
      const ui::LatencyInfo& latency,
      mojom::WidgetInputHandler::DispatchEventCallback callback);
  void DidHandleInputEventAndOverscroll(
      mojom::WidgetInputHandler::DispatchEventCallback callback,
      ui::InputHandlerProxy::EventDisposition event_disposition,
      ui::WebScopedInputEvent input_event,
      const ui::LatencyInfo& latency_info,
      std::unique_ptr<ui::DidOverscrollParams> overscroll_params);
  void HandledInputEvent(
      mojom::WidgetInputHandler::DispatchEventCallback callback,
      InputEventAckState ack_state,
      const ui::LatencyInfo& latency_info,
      std::unique_ptr<ui::DidOverscrollParams> overscroll_params,
      base::Optional<cc::TouchAction> touch_action);
  void ObserveGestureEventOnCompositorThread(
      const blink::WebGestureEvent& gesture_event,
      const cc::InputHandlerScrollResult& scroll_result);

  // Only valid to be called on the main thread.
  base::WeakPtr<RenderWidget> render_widget_;
  blink::scheduler::WebMainThreadScheduler* main_thread_scheduler_;

  // InputHandlerProxy is only interacted with on the compositor
  // thread.
  std::unique_ptr<ui::InputHandlerProxy> input_handler_proxy_;

  using WidgetInputHandlerHost = scoped_refptr<
      mojo::ThreadSafeInterfacePtr<mojom::WidgetInputHandlerHost>>;

  // The WidgetInputHandlerHost is bound on the compositor task runner
  // but class can be called on the compositor and main thread.
  WidgetInputHandlerHost host_;

  // Host that was passed as part of the FrameInputHandler associated
  // channel.
  WidgetInputHandlerHost associated_host_;

  // Any thread can access these variables.
  scoped_refptr<MainThreadEventQueue> input_event_queue_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

#if defined(OS_ANDROID)
  std::unique_ptr<SynchronousCompositorProxyRegistry>
      synchronous_compositor_registry_;
#endif

  DISALLOW_COPY_AND_ASSIGN(WidgetInputHandlerManager);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_WIDGET_INPUT_HANDLER_MANAGER_H_
