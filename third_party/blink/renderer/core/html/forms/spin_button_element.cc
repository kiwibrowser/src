/*
 * Copyright (C) 2006, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/forms/spin_button_element.h"

#include "build/build_config.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/events/wheel_event.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/shadow/shadow_element_names.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_theme.h"

namespace blink {

using namespace HTMLNames;

inline SpinButtonElement::SpinButtonElement(Document& document,
                                            SpinButtonOwner& spin_button_owner)
    : HTMLDivElement(document),
      spin_button_owner_(&spin_button_owner),
      capturing_(false),
      up_down_state_(kIndeterminate),
      press_starting_state_(kIndeterminate),
      repeating_timer_(document.GetTaskRunner(TaskType::kInternalDefault),
                       this,
                       &SpinButtonElement::RepeatingTimerFired) {}

SpinButtonElement* SpinButtonElement::Create(
    Document& document,
    SpinButtonOwner& spin_button_owner) {
  SpinButtonElement* element =
      new SpinButtonElement(document, spin_button_owner);
  element->SetShadowPseudoId(AtomicString("-webkit-inner-spin-button"));
  element->setAttribute(idAttr, ShadowElementNames::SpinButton());
  return element;
}

void SpinButtonElement::DetachLayoutTree(const AttachContext& context) {
  ReleaseCapture(kEventDispatchDisallowed);
  HTMLDivElement::DetachLayoutTree(context);
}

void SpinButtonElement::DefaultEventHandler(Event* event) {
  if (!event->IsMouseEvent()) {
    if (!event->DefaultHandled())
      HTMLDivElement::DefaultEventHandler(event);
    return;
  }

  LayoutBox* box = GetLayoutBox();
  if (!box) {
    if (!event->DefaultHandled())
      HTMLDivElement::DefaultEventHandler(event);
    return;
  }

  if (!ShouldRespondToMouseEvents()) {
    if (!event->DefaultHandled())
      HTMLDivElement::DefaultEventHandler(event);
    return;
  }

  MouseEvent* mouse_event = ToMouseEvent(event);
  IntPoint local = RoundedIntPoint(box->AbsoluteToLocal(
      FloatPoint(mouse_event->AbsoluteLocation()), kUseTransforms));
  if (mouse_event->type() == EventTypeNames::mousedown &&
      mouse_event->button() ==
          static_cast<short>(WebPointerProperties::Button::kLeft)) {
    if (box->PixelSnappedBorderBoxRect().Contains(local)) {
      if (spin_button_owner_)
        spin_button_owner_->FocusAndSelectSpinButtonOwner();
      if (GetLayoutObject()) {
        if (up_down_state_ != kIndeterminate) {
          // A JavaScript event handler called in doStepAction() below
          // might change the element state and we might need to
          // cancel the repeating timer by the state change. If we
          // started the timer after doStepAction(), we would have no
          // chance to cancel the timer.
          StartRepeatingTimer();
          DoStepAction(up_down_state_ == kUp ? 1 : -1);
        }
      }
      event->SetDefaultHandled();
    }
  } else if (mouse_event->type() == EventTypeNames::mouseup &&
             mouse_event->button() ==
                 static_cast<short>(WebPointerProperties::Button::kLeft)) {
    ReleaseCapture();
  } else if (event->type() == EventTypeNames::mousemove) {
    if (box->PixelSnappedBorderBoxRect().Contains(local)) {
      if (!capturing_) {
        if (LocalFrame* frame = GetDocument().GetFrame()) {
          frame->GetEventHandler().SetCapturingMouseEventsNode(this);
          capturing_ = true;
          if (Page* page = GetDocument().GetPage())
            page->GetChromeClient().RegisterPopupOpeningObserver(this);
        }
      }
      UpDownState old_up_down_state = up_down_state_;
      up_down_state_ = (local.Y() < box->Size().Height() / 2) ? kUp : kDown;
      if (up_down_state_ != old_up_down_state)
        GetLayoutObject()->SetShouldDoFullPaintInvalidation();
    } else {
      ReleaseCapture();
      up_down_state_ = kIndeterminate;
    }
  }

  if (!event->DefaultHandled())
    HTMLDivElement::DefaultEventHandler(event);
}

void SpinButtonElement::WillOpenPopup() {
  ReleaseCapture();
  up_down_state_ = kIndeterminate;
}

void SpinButtonElement::ForwardEvent(Event* event) {
  if (!GetLayoutBox())
    return;

  if (!event->HasInterface(EventNames::WheelEvent))
    return;

  if (!spin_button_owner_)
    return;

  if (!spin_button_owner_->ShouldSpinButtonRespondToWheelEvents())
    return;

  DoStepAction(ToWheelEvent(event)->wheelDeltaY());
  event->SetDefaultHandled();
}

bool SpinButtonElement::WillRespondToMouseMoveEvents() {
  if (GetLayoutBox() && ShouldRespondToMouseEvents())
    return true;

  return HTMLDivElement::WillRespondToMouseMoveEvents();
}

bool SpinButtonElement::WillRespondToMouseClickEvents() {
  if (GetLayoutBox() && ShouldRespondToMouseEvents())
    return true;

  return HTMLDivElement::WillRespondToMouseClickEvents();
}

void SpinButtonElement::DoStepAction(int amount) {
  if (!spin_button_owner_)
    return;

  if (amount > 0)
    spin_button_owner_->SpinButtonStepUp();
  else if (amount < 0)
    spin_button_owner_->SpinButtonStepDown();
}

void SpinButtonElement::ReleaseCapture(EventDispatch event_dispatch) {
  StopRepeatingTimer();
  if (!capturing_)
    return;
  if (LocalFrame* frame = GetDocument().GetFrame()) {
    frame->GetEventHandler().SetCapturingMouseEventsNode(nullptr);
    capturing_ = false;
    if (Page* page = GetDocument().GetPage())
      page->GetChromeClient().UnregisterPopupOpeningObserver(this);
  }
  if (spin_button_owner_)
    spin_button_owner_->SpinButtonDidReleaseMouseCapture(event_dispatch);
}

bool SpinButtonElement::MatchesReadOnlyPseudoClass() const {
  return OwnerShadowHost()->MatchesReadOnlyPseudoClass();
}

bool SpinButtonElement::MatchesReadWritePseudoClass() const {
  return OwnerShadowHost()->MatchesReadWritePseudoClass();
}

void SpinButtonElement::StartRepeatingTimer() {
  press_starting_state_ = up_down_state_;
  Page* page = GetDocument().GetPage();
  DCHECK(page);
  ScrollbarTheme& theme = page->GetScrollbarTheme();
  repeating_timer_.Start(theme.InitialAutoscrollTimerDelay(),
                         theme.AutoscrollTimerDelay(), FROM_HERE);
}

void SpinButtonElement::StopRepeatingTimer() {
  repeating_timer_.Stop();
}

void SpinButtonElement::Step(int amount) {
  if (!ShouldRespondToMouseEvents())
    return;
// On Mac OS, NSStepper updates the value for the button under the mouse
// cursor regardless of the button pressed at the beginning. So the
// following check is not needed for Mac OS.
#if !defined(OS_MACOSX)
  if (up_down_state_ != press_starting_state_)
    return;
#endif
  DoStepAction(amount);
}

void SpinButtonElement::RepeatingTimerFired(TimerBase*) {
  if (up_down_state_ != kIndeterminate)
    Step(up_down_state_ == kUp ? 1 : -1);
}

void SpinButtonElement::SetHovered(bool flag) {
  if (!flag)
    up_down_state_ = kIndeterminate;
  HTMLDivElement::SetHovered(flag);
}

bool SpinButtonElement::ShouldRespondToMouseEvents() {
  return !spin_button_owner_ ||
         spin_button_owner_->ShouldSpinButtonRespondToMouseEvents();
}

void SpinButtonElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(spin_button_owner_);
  HTMLDivElement::Trace(visitor);
}

}  // namespace blink
