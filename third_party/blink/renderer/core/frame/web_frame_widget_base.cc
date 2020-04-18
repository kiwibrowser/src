// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/web_frame_widget_base.h"

#include <memory>
#include <utility>

#include "base/time/time.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_gesture_curve.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_widget_client.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/user_gesture_indicator.h"
#include "third_party/blink/renderer/core/events/web_input_event_conversion.h"
#include "third_party/blink/renderer/core/events/wheel_event.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/input/context_menu_allowed_scope.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/page/context_menu_controller.h"
#include "third_party/blink/renderer/core/page/drag_actions.h"
#include "third_party/blink/renderer/core/page/drag_controller.h"
#include "third_party/blink/renderer/core/page/drag_data.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/pointer_lock_controller.h"
#include "third_party/blink/renderer/platform/exported/web_active_gesture_animation.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

// Ensure that the WebDragOperation enum values stay in sync with the original
// DragOperation constants.
STATIC_ASSERT_ENUM(kDragOperationNone, kWebDragOperationNone);
STATIC_ASSERT_ENUM(kDragOperationCopy, kWebDragOperationCopy);
STATIC_ASSERT_ENUM(kDragOperationLink, kWebDragOperationLink);
STATIC_ASSERT_ENUM(kDragOperationGeneric, kWebDragOperationGeneric);
STATIC_ASSERT_ENUM(kDragOperationPrivate, kWebDragOperationPrivate);
STATIC_ASSERT_ENUM(kDragOperationMove, kWebDragOperationMove);
STATIC_ASSERT_ENUM(kDragOperationDelete, kWebDragOperationDelete);
STATIC_ASSERT_ENUM(kDragOperationEvery, kWebDragOperationEvery);

bool WebFrameWidgetBase::ignore_input_events_ = false;

WebFrameWidgetBase::WebFrameWidgetBase(WebWidgetClient& client)
    : client_(&client),
      fling_modifier_(0),
      fling_source_device_(kWebGestureDeviceUninitialized) {}

WebFrameWidgetBase::~WebFrameWidgetBase() = default;

void WebFrameWidgetBase::BindLocalRoot(WebLocalFrame& local_root) {
  local_root_ = ToWebLocalFrameImpl(local_root);
  local_root_->SetFrameWidget(this);

  Initialize();
}

void WebFrameWidgetBase::Close() {
  local_root_->SetFrameWidget(nullptr);
  local_root_ = nullptr;
  client_ = nullptr;
}

WebLocalFrame* WebFrameWidgetBase::LocalRoot() const {
  return local_root_;
}

WebDragOperation WebFrameWidgetBase::DragTargetDragEnter(
    const WebDragData& web_drag_data,
    const WebFloatPoint& point_in_viewport,
    const WebFloatPoint& screen_point,
    WebDragOperationsMask operations_allowed,
    int modifiers) {
  DCHECK(!current_drag_data_);

  current_drag_data_ = DataObject::Create(web_drag_data);
  operations_allowed_ = operations_allowed;

  return DragTargetDragEnterOrOver(point_in_viewport, screen_point, kDragEnter,
                                   modifiers);
}

WebDragOperation WebFrameWidgetBase::DragTargetDragOver(
    const WebFloatPoint& point_in_viewport,
    const WebFloatPoint& screen_point,
    WebDragOperationsMask operations_allowed,
    int modifiers) {
  operations_allowed_ = operations_allowed;

  return DragTargetDragEnterOrOver(point_in_viewport, screen_point, kDragOver,
                                   modifiers);
}

void WebFrameWidgetBase::DragTargetDragLeave(
    const WebFloatPoint& point_in_viewport,
    const WebFloatPoint& screen_point) {
  DCHECK(current_drag_data_);

  // TODO(paulmeyer): It shouldn't be possible for |m_currentDragData| to be
  // null here, but this is somehow happening (rarely). This suggests that in
  // some cases drag-leave is happening before drag-enter, which should be
  // impossible. This needs to be investigated further. Once fixed, the extra
  // check for |!m_currentDragData| should be removed. (crbug.com/671152)
  if (IgnoreInputEvents() || !current_drag_data_) {
    CancelDrag();
    return;
  }

  WebFloatPoint point_in_root_frame(ViewportToRootFrame(point_in_viewport));
  DragData drag_data(current_drag_data_.Get(), point_in_root_frame,
                     screen_point,
                     static_cast<DragOperation>(operations_allowed_));

  GetPage()->GetDragController().DragExited(&drag_data,
                                            *local_root_->GetFrame());

  // FIXME: why is the drag scroll timer not stopped here?

  drag_operation_ = kWebDragOperationNone;
  current_drag_data_ = nullptr;
}

void WebFrameWidgetBase::DragTargetDrop(const WebDragData& web_drag_data,
                                        const WebFloatPoint& point_in_viewport,
                                        const WebFloatPoint& screen_point,
                                        int modifiers) {
  WebFloatPoint point_in_root_frame(ViewportToRootFrame(point_in_viewport));

  DCHECK(current_drag_data_);
  current_drag_data_ = DataObject::Create(web_drag_data);

  // If this webview transitions from the "drop accepting" state to the "not
  // accepting" state, then our IPC message reply indicating that may be in-
  // flight, or else delayed by javascript processing in this webview.  If a
  // drop happens before our IPC reply has reached the browser process, then
  // the browser forwards the drop to this webview.  So only allow a drop to
  // proceed if our webview m_dragOperation state is not DragOperationNone.

  if (drag_operation_ == kWebDragOperationNone) {
    // IPC RACE CONDITION: do not allow this drop.
    DragTargetDragLeave(point_in_viewport, screen_point);
    return;
  }

  if (!IgnoreInputEvents()) {
    current_drag_data_->SetModifiers(modifiers);
    DragData drag_data(current_drag_data_.Get(), point_in_root_frame,
                       screen_point,
                       static_cast<DragOperation>(operations_allowed_));

    GetPage()->GetDragController().PerformDrag(&drag_data,
                                               *local_root_->GetFrame());
  }
  drag_operation_ = kWebDragOperationNone;
  current_drag_data_ = nullptr;
}

void WebFrameWidgetBase::DragSourceEndedAt(
    const WebFloatPoint& point_in_viewport,
    const WebFloatPoint& screen_point,
    WebDragOperation operation) {
  if (!local_root_) {
    // We should figure out why |local_root_| could be nullptr
    // (https://crbug.com/792345).
    return;
  }

  if (IgnoreInputEvents()) {
    CancelDrag();
    return;
  }
  WebFloatPoint point_in_root_frame(
      GetPage()->GetVisualViewport().ViewportToRootFrame(point_in_viewport));

  WebMouseEvent fake_mouse_move(
      WebInputEvent::kMouseMove, point_in_root_frame, screen_point,
      WebPointerProperties::Button::kLeft, 0, WebInputEvent::kNoModifiers,
      CurrentTimeTicks());
  fake_mouse_move.SetFrameScale(1);
  local_root_->GetFrame()->GetEventHandler().DragSourceEndedAt(
      fake_mouse_move, static_cast<DragOperation>(operation));
}

void WebFrameWidgetBase::DragSourceSystemDragEnded() {
  CancelDrag();
}

void WebFrameWidgetBase::CompositeWithRasterForTesting() {
  if (auto* layer_tree_view = GetLayerTreeView())
    layer_tree_view->CompositeWithRasterForTesting();
}

void WebFrameWidgetBase::CancelDrag() {
  // It's possible for us this to be callback while we're not doing a drag if
  // it's from a previous page that got unloaded.
  if (!doing_drag_and_drop_)
    return;
  GetPage()->GetDragController().DragEnded();
  doing_drag_and_drop_ = false;
}

void WebFrameWidgetBase::StartDragging(WebReferrerPolicy policy,
                                       const WebDragData& data,
                                       WebDragOperationsMask mask,
                                       const WebImage& drag_image,
                                       const WebPoint& drag_image_offset) {
  doing_drag_and_drop_ = true;
  Client()->StartDragging(policy, data, mask, drag_image, drag_image_offset);
}

WebDragOperation WebFrameWidgetBase::DragTargetDragEnterOrOver(
    const WebFloatPoint& point_in_viewport,
    const WebFloatPoint& screen_point,
    DragAction drag_action,
    int modifiers) {
  DCHECK(current_drag_data_);
  // TODO(paulmeyer): It shouldn't be possible for |m_currentDragData| to be
  // null here, but this is somehow happening (rarely). This suggests that in
  // some cases drag-over is happening before drag-enter, which should be
  // impossible. This needs to be investigated further. Once fixed, the extra
  // check for |!m_currentDragData| should be removed. (crbug.com/671504)
  if (IgnoreInputEvents() || !current_drag_data_) {
    CancelDrag();
    return kWebDragOperationNone;
  }

  WebFloatPoint point_in_root_frame(ViewportToRootFrame(point_in_viewport));

  current_drag_data_->SetModifiers(modifiers);
  DragData drag_data(current_drag_data_.Get(), point_in_root_frame,
                     screen_point,
                     static_cast<DragOperation>(operations_allowed_));

  DragOperation drag_operation =
      GetPage()->GetDragController().DragEnteredOrUpdated(
          &drag_data, *local_root_->GetFrame());

  // Mask the drag operation against the drag source's allowed
  // operations.
  if (!(drag_operation & drag_data.DraggingSourceOperationMask()))
    drag_operation = kDragOperationNone;

  drag_operation_ = static_cast<WebDragOperation>(drag_operation);

  return drag_operation_;
}

WebFloatPoint WebFrameWidgetBase::ViewportToRootFrame(
    const WebFloatPoint& point_in_viewport) const {
  return GetPage()->GetVisualViewport().ViewportToRootFrame(point_in_viewport);
}

WebViewImpl* WebFrameWidgetBase::View() const {
  return local_root_->ViewImpl();
}

Page* WebFrameWidgetBase::GetPage() const {
  return View()->GetPage();
}

void WebFrameWidgetBase::DidAcquirePointerLock() {
  GetPage()->GetPointerLockController().DidAcquirePointerLock();

  LocalFrame* focusedFrame = FocusedLocalFrameInWidget();
  if (focusedFrame) {
    focusedFrame->GetEventHandler().ReleaseMousePointerCapture();
  }
}

void WebFrameWidgetBase::DidNotAcquirePointerLock() {
  GetPage()->GetPointerLockController().DidNotAcquirePointerLock();
}

void WebFrameWidgetBase::DidLosePointerLock() {
  pointer_lock_gesture_token_ = nullptr;
  GetPage()->GetPointerLockController().DidLosePointerLock();
}

void WebFrameWidgetBase::RequestDecode(
    const PaintImage& image,
    base::OnceCallback<void(bool)> callback) {
  // If we have a LayerTreeView, propagate the request, otherwise fail it since
  // otherwise it would remain in a unresolved and unrejected state.
  if (WebLayerTreeView* layer_tree_view = GetLayerTreeView()) {
    layer_tree_view->RequestDecode(image, std::move(callback));
  } else {
    std::move(callback).Run(false);
  }
}

void WebFrameWidgetBase::Trace(blink::Visitor* visitor) {
  visitor->Trace(local_root_);
  visitor->Trace(current_drag_data_);
}

// TODO(665924): Remove direct dispatches of mouse events from
// PointerLockController, instead passing them through EventHandler.
void WebFrameWidgetBase::PointerLockMouseEvent(
    const WebCoalescedInputEvent& coalesced_event) {
  const WebInputEvent& input_event = coalesced_event.Event();
  const WebMouseEvent& mouse_event =
      static_cast<const WebMouseEvent&>(input_event);
  WebMouseEvent transformed_event =
      TransformWebMouseEvent(local_root_->GetFrameView(), mouse_event);

  LocalFrame* focusedFrame = FocusedLocalFrameInWidget();
  if (focusedFrame) {
    focusedFrame->GetEventHandler().ProcessPendingPointerCaptureForPointerLock(
        transformed_event);
  }

  std::unique_ptr<UserGestureIndicator> gesture_indicator;
  AtomicString event_type;
  switch (input_event.GetType()) {
    case WebInputEvent::kMouseDown:
      event_type = EventTypeNames::mousedown;
      if (!GetPage() || !GetPage()->GetPointerLockController().GetElement())
        break;
      gesture_indicator =
          Frame::NotifyUserActivation(GetPage()
                                          ->GetPointerLockController()
                                          .GetElement()
                                          ->GetDocument()
                                          .GetFrame(),
                                      UserGestureToken::kNewGesture);
      pointer_lock_gesture_token_ = gesture_indicator->CurrentToken();
      break;
    case WebInputEvent::kMouseUp:
      event_type = EventTypeNames::mouseup;
      gesture_indicator = std::make_unique<UserGestureIndicator>(
          std::move(pointer_lock_gesture_token_));
      break;
    case WebInputEvent::kMouseMove:
      event_type = EventTypeNames::mousemove;
      break;
    default:
      NOTREACHED() << input_event.GetType();
  }

  if (GetPage()) {
    GetPage()->GetPointerLockController().DispatchLockedMouseEvent(
        transformed_event, event_type);
  }
}

void WebFrameWidgetBase::ShowContextMenu(WebMenuSourceType source_type) {
  if (!GetPage())
    return;

  GetPage()->GetContextMenuController().ClearContextMenu();
  {
    ContextMenuAllowedScope scope;
    if (LocalFrame* focused_frame =
            GetPage()->GetFocusController().FocusedFrame()) {
      focused_frame->GetEventHandler().ShowNonLocatedContextMenu(nullptr,
                                                                 source_type);
    }
  }
}

LocalFrame* WebFrameWidgetBase::FocusedLocalFrameInWidget() const {
  if (!local_root_) {
    // WebFrameWidget is created in the call to CreateFrame. The corresponding
    // RenderWidget, however, might not swap in right away (InstallNewDocument()
    // will lead to it swapping in). During this interval local_root_ is nullptr
    // (see https://crbug.com/792345).
    return nullptr;
  }

  LocalFrame* frame = GetPage()->GetFocusController().FocusedFrame();
  return (frame && frame->LocalFrameRoot() == local_root_->GetFrame())
             ? frame
             : nullptr;
}

bool WebFrameWidgetBase::EndActiveFlingAnimation() {
  if (gesture_animation_) {
    gesture_animation_.reset();
    fling_source_device_ = kWebGestureDeviceUninitialized;
    if (WebLayerTreeView* layer_tree_view = GetLayerTreeView())
      layer_tree_view->DidStopFlinging();
    return true;
  }
  return false;
}

bool WebFrameWidgetBase::ScrollBy(const WebFloatSize& delta,
                                  const WebFloatSize& velocity) {
  DCHECK_NE(fling_source_device_, kWebGestureDeviceUninitialized);

  if (fling_source_device_ == kWebGestureDeviceTouchpad) {
    bool enable_touchpad_scroll_latching =
        RuntimeEnabledFeatures::TouchpadAndWheelScrollLatchingEnabled();
    WebMouseWheelEvent synthetic_wheel(
        WebInputEvent::kMouseWheel, fling_modifier_, WTF::CurrentTimeTicks());
    const float kTickDivisor = WheelEvent::kTickMultiplier;

    synthetic_wheel.delta_x = delta.width;
    synthetic_wheel.delta_y = delta.height;
    synthetic_wheel.wheel_ticks_x = delta.width / kTickDivisor;
    synthetic_wheel.wheel_ticks_y = delta.height / kTickDivisor;
    synthetic_wheel.has_precise_scrolling_deltas = true;
    synthetic_wheel.phase = WebMouseWheelEvent::kPhaseChanged;
    synthetic_wheel.SetPositionInWidget(position_on_fling_start_.x,
                                        position_on_fling_start_.y);
    synthetic_wheel.SetPositionInScreen(global_position_on_fling_start_.x,
                                        global_position_on_fling_start_.y);

    // TODO(wjmaclean): Is local_root_ the right frame to use here?
    if (GetPageWidgetEventHandler()->HandleMouseWheel(*local_root_->GetFrame(),
                                                      synthetic_wheel) !=
        WebInputEventResult::kNotHandled) {
      return true;
    }

    if (!enable_touchpad_scroll_latching) {
      WebGestureEvent synthetic_scroll_begin =
          CreateGestureScrollEventFromFling(WebInputEvent::kGestureScrollBegin,
                                            kWebGestureDeviceTouchpad);
      synthetic_scroll_begin.data.scroll_begin.delta_x_hint = delta.width;
      synthetic_scroll_begin.data.scroll_begin.delta_y_hint = delta.height;
      synthetic_scroll_begin.data.scroll_begin.inertial_phase =
          WebGestureEvent::kMomentumPhase;
      GetPageWidgetEventHandler()->HandleGestureEvent(synthetic_scroll_begin);
    }

    WebGestureEvent synthetic_scroll_update = CreateGestureScrollEventFromFling(
        WebInputEvent::kGestureScrollUpdate, kWebGestureDeviceTouchpad);
    synthetic_scroll_update.data.scroll_update.delta_x = delta.width;
    synthetic_scroll_update.data.scroll_update.delta_y = delta.height;
    synthetic_scroll_update.data.scroll_update.velocity_x = velocity.width;
    synthetic_scroll_update.data.scroll_update.velocity_y = velocity.height;
    synthetic_scroll_update.data.scroll_update.inertial_phase =
        WebGestureEvent::kMomentumPhase;
    bool scroll_update_handled =
        GetPageWidgetEventHandler()->HandleGestureEvent(
            synthetic_scroll_update) != WebInputEventResult::kNotHandled;

    if (!enable_touchpad_scroll_latching) {
      WebGestureEvent synthetic_scroll_end = CreateGestureScrollEventFromFling(
          WebInputEvent::kGestureScrollEnd, kWebGestureDeviceTouchpad);
      synthetic_scroll_end.data.scroll_end.inertial_phase =
          WebGestureEvent::kMomentumPhase;
      GetPageWidgetEventHandler()->HandleGestureEvent(synthetic_scroll_end);
    }

    return scroll_update_handled;
  }

  WebGestureEvent synthetic_gesture_event = CreateGestureScrollEventFromFling(
      WebInputEvent::kGestureScrollUpdate, fling_source_device_);
  synthetic_gesture_event.data.scroll_update.delta_x = delta.width;
  synthetic_gesture_event.data.scroll_update.delta_y = delta.height;
  synthetic_gesture_event.data.scroll_update.velocity_x = velocity.width;
  synthetic_gesture_event.data.scroll_update.velocity_y = velocity.height;
  synthetic_gesture_event.data.scroll_update.inertial_phase =
      WebGestureEvent::kMomentumPhase;

  return GetPageWidgetEventHandler()->HandleGestureEvent(
             synthetic_gesture_event) != WebInputEventResult::kNotHandled;
}

WebInputEventResult WebFrameWidgetBase::HandleGestureFlingEvent(
    const WebGestureEvent& event) {
  WebInputEventResult event_result = WebInputEventResult::kNotHandled;
  switch (event.GetType()) {
    case WebInputEvent::kGestureFlingStart: {
      if (event.SourceDevice() != kWebGestureDeviceSyntheticAutoscroll)
        EndActiveFlingAnimation();
      position_on_fling_start_ = event.PositionInWidget();
      global_position_on_fling_start_ = event.PositionInScreen();
      fling_modifier_ = event.GetModifiers();
      fling_source_device_ = event.SourceDevice();
      DCHECK_NE(fling_source_device_, kWebGestureDeviceUninitialized);
      std::unique_ptr<WebGestureCurve> fling_curve =
          Platform::Current()->CreateFlingAnimationCurve(
              event.SourceDevice(),
              WebFloatPoint(event.data.fling_start.velocity_x,
                            event.data.fling_start.velocity_y),
              WebSize());
      DCHECK(fling_curve);
      gesture_animation_ = WebActiveGestureAnimation::CreateWithTimeOffset(
          std::move(fling_curve), this, event.TimeStamp());
      ScheduleAnimation();

      WebGestureEvent scaled_event =
          TransformWebGestureEvent(local_root_->GetFrameView(), event);
      // Plugins may need to see GestureFlingStart to balance
      // GestureScrollBegin (since the former replaces GestureScrollEnd when
      // transitioning to a fling).
      // TODO(dtapuska): Why isn't the response used?
      local_root_->GetFrame()->GetEventHandler().HandleGestureScrollEvent(
          scaled_event);

      event_result = WebInputEventResult::kHandledSystem;
      break;
    }
    case WebInputEvent::kGestureFlingCancel:
      if (EndActiveFlingAnimation())
        event_result = WebInputEventResult::kHandledSuppressed;

      break;
    default:
      NOTREACHED();
  }
  return event_result;
}

WebLocalFrame* WebFrameWidgetBase::FocusedWebLocalFrameInWidget() const {
  return WebLocalFrameImpl::FromFrame(FocusedLocalFrameInWidget());
}

WebGestureEvent WebFrameWidgetBase::CreateGestureScrollEventFromFling(
    WebInputEvent::Type type,
    WebGestureDevice source_device) const {
  WebGestureEvent gesture_event(type, fling_modifier_, WTF::CurrentTimeTicks(),
                                source_device);
  gesture_event.SetPositionInWidget(position_on_fling_start_);
  gesture_event.SetPositionInScreen(global_position_on_fling_start_);
  return gesture_event;
}

bool WebFrameWidgetBase::IsFlinging() const {
  return !!gesture_animation_;
}

void WebFrameWidgetBase::UpdateGestureAnimation(
    base::TimeTicks last_frame_time) {
  if (!gesture_animation_)
    return;

  if (gesture_animation_->Animate(last_frame_time)) {
    ScheduleAnimation();
  } else {
    DCHECK_NE(fling_source_device_, kWebGestureDeviceUninitialized);
    WebGestureDevice last_fling_source_device = fling_source_device_;
    EndActiveFlingAnimation();

    if (last_fling_source_device != kWebGestureDeviceSyntheticAutoscroll) {
      WebGestureEvent end_scroll_event = CreateGestureScrollEventFromFling(
          WebInputEvent::kGestureScrollEnd, last_fling_source_device);
      local_root_->GetFrame()->GetEventHandler().HandleGestureScrollEnd(
          end_scroll_event);
    }
  }
}

}  // namespace blink
