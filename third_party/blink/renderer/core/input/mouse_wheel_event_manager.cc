// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/input/mouse_wheel_event_manager.h"

#include "build/build_config.h"
#include "third_party/blink/public/platform/web_mouse_wheel_event.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/events/wheel_event.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/input/event_handling_util.h"
#include "third_party/blink/renderer/core/layout/hit_test_request.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"

namespace blink {
MouseWheelEventManager::MouseWheelEventManager(LocalFrame& frame)
    : frame_(frame), wheel_target_(nullptr) {}

void MouseWheelEventManager::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_);
  visitor->Trace(wheel_target_);
}

void MouseWheelEventManager::Clear() {
  wheel_target_ = nullptr;
}

WebInputEventResult MouseWheelEventManager::HandleWheelEvent(
    const WebMouseWheelEvent& event) {
  bool wheel_scroll_latching =
      RuntimeEnabledFeatures::TouchpadAndWheelScrollLatchingEnabled();

  Document* doc = frame_->GetDocument();
  if (!doc || !doc->GetLayoutView())
    return WebInputEventResult::kNotHandled;

  LocalFrameView* view = frame_->View();
  if (!view)
    return WebInputEventResult::kNotHandled;

  if (wheel_scroll_latching) {
    const int kWheelEventPhaseEndedEventMask =
        WebMouseWheelEvent::kPhaseEnded | WebMouseWheelEvent::kPhaseCancelled;
    const int kWheelEventPhaseNoEventMask =
        kWheelEventPhaseEndedEventMask | WebMouseWheelEvent::kPhaseMayBegin;

    if ((event.phase & kWheelEventPhaseEndedEventMask) ||
        (event.momentum_phase & kWheelEventPhaseEndedEventMask)) {
      wheel_target_ = nullptr;
    }

    if ((event.phase & kWheelEventPhaseNoEventMask) ||
        (event.momentum_phase & kWheelEventPhaseNoEventMask)) {
      // Filter wheel events with zero deltas and reset the wheel_target_ node.
      DCHECK(!event.delta_x && !event.delta_y);
      return WebInputEventResult::kNotHandled;
    }

    bool has_phase_info =
        event.phase != WebMouseWheelEvent::kPhaseNone ||
        event.momentum_phase != WebMouseWheelEvent::kPhaseNone;
    if (!has_phase_info) {
      // Synthetic wheel events generated from GesturePinchUpdate don't have
      // phase info. Send these events to the target under the cursor.
      wheel_target_ = FindTargetNode(event, doc, view);
    } else if (event.phase == WebMouseWheelEvent::kPhaseBegan ||
               !wheel_target_) {
      // Find and save the wheel_target_, this target will be used for the rest
      // of the current scrolling sequence.
      wheel_target_ = FindTargetNode(event, doc, view);
    }
  } else {  // !wheel_scroll_latching, wheel_target_ will be updated for each
            // wheel event.
#if defined(OS_MACOSX)
    // Filter Mac OS specific phases, usually with a zero-delta.
    // https://crbug.com/553732
    // TODO(chongz): EventSender sends events with
    // |WebMouseWheelEvent::PhaseNone|,
    // but it shouldn't.
    const int kWheelEventPhaseNoEventMask =
        WebMouseWheelEvent::kPhaseEnded | WebMouseWheelEvent::kPhaseCancelled |
        WebMouseWheelEvent::kPhaseMayBegin;
    if ((event.phase & kWheelEventPhaseNoEventMask) ||
        (event.momentum_phase & kWheelEventPhaseNoEventMask))
      return WebInputEventResult::kNotHandled;
#endif

    wheel_target_ = FindTargetNode(event, doc, view);
  }

  LocalFrame* subframe =
      EventHandlingUtil::SubframeForTargetNode(wheel_target_.Get());
  if (subframe) {
    WebInputEventResult result =
        subframe->GetEventHandler().HandleWheelEvent(event);
    return result;
  }

  if (wheel_target_) {
    WheelEvent* dom_event =
        WheelEvent::Create(event, wheel_target_->GetDocument().domWindow());
    DispatchEventResult dom_event_result =
        wheel_target_->DispatchEvent(dom_event);
    if (dom_event_result != DispatchEventResult::kNotCanceled)
      return EventHandlingUtil::ToWebInputEventResult(dom_event_result);
  }

  return WebInputEventResult::kNotHandled;
}

void MouseWheelEventManager::ElementRemoved(Node* target) {
  if (wheel_target_ == target)
    wheel_target_ = nullptr;
}

Node* MouseWheelEventManager::FindTargetNode(const WebMouseWheelEvent& event,
                                             const Document* doc,
                                             const LocalFrameView* view) {
  DCHECK(doc && doc->GetLayoutView() && view);
  LayoutPoint v_point =
      view->RootFrameToContents(FlooredIntPoint(event.PositionInRootFrame()));

  HitTestRequest request(HitTestRequest::kReadOnly);
  HitTestResult result(request, v_point);
  doc->GetLayoutView()->HitTest(result);

  Node* node = result.InnerNode();
  // Wheel events should not dispatch to text nodes.
  if (node && node->IsTextNode())
    node = FlatTreeTraversal::Parent(*node);

  // If we're over the frame scrollbar, scroll the document.
  if (!node && result.GetScrollbar())
    node = doc->documentElement();

  return node;
}

}  // namespace blink
