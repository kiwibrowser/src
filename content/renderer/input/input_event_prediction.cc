// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/input_event_prediction.h"

#include "base/feature_list.h"
#include "content/public/common/content_features.h"
#include "ui/events/blink/prediction/empty_predictor.h"

using blink::WebInputEvent;
using blink::WebMouseEvent;
using blink::WebPointerEvent;
using blink::WebPointerProperties;
using blink::WebTouchEvent;

namespace content {

namespace {

std::unique_ptr<ui::InputPredictor> SetUpPredictor() {
  return std::make_unique<ui::EmptyPredictor>();
}

}  // namespace

InputEventPrediction::InputEventPrediction() {
  mouse_predictor_ = SetUpPredictor();
}

InputEventPrediction::~InputEventPrediction() {}

void InputEventPrediction::HandleEvents(
    const blink::WebCoalescedInputEvent& coalesced_event,
    base::TimeTicks frame_time,
    blink::WebInputEvent* event) {
  switch (event->GetType()) {
    case WebInputEvent::kMouseMove:
    case WebInputEvent::kTouchMove:
    case WebInputEvent::kPointerMove: {
      size_t coalesced_size = coalesced_event.CoalescedEventSize();
      for (size_t i = 0; i < coalesced_size; i++)
        UpdatePrediction(coalesced_event.CoalescedEvent(i));

      ApplyResampling(frame_time, event);
      break;
    }
    case WebInputEvent::kTouchScrollStarted:
    case WebInputEvent::kPointerCausedUaAction:
      pointer_id_predictor_map_.clear();
      break;
    default:
      ResetPredictor(*event);
  }
}

void InputEventPrediction::UpdatePrediction(const WebInputEvent& event) {
  if (WebInputEvent::IsTouchEventType(event.GetType())) {
    DCHECK(event.GetType() == WebInputEvent::kTouchMove);
    const WebTouchEvent& touch_event = static_cast<const WebTouchEvent&>(event);
    for (unsigned i = 0; i < touch_event.touches_length; ++i) {
      if (touch_event.touches[i].state == blink::WebTouchPoint::kStateMoved) {
        UpdateSinglePointer(touch_event.touches[i], touch_event.TimeStamp());
      }
    }
  } else if (WebInputEvent::IsMouseEventType(event.GetType())) {
    DCHECK(event.GetType() == WebInputEvent::kMouseMove);
    UpdateSinglePointer(static_cast<const WebMouseEvent&>(event),
                        event.TimeStamp());
  } else if (WebInputEvent::IsPointerEventType(event.GetType())) {
    DCHECK(event.GetType() == WebInputEvent::kPointerMove);
    UpdateSinglePointer(static_cast<const WebPointerEvent&>(event),
                        event.TimeStamp());
  }
}

void InputEventPrediction::ApplyResampling(base::TimeTicks frame_time,
                                           WebInputEvent* event) {
  if (event->GetType() == WebInputEvent::kTouchMove) {
    WebTouchEvent* touch_event = static_cast<WebTouchEvent*>(event);
    for (unsigned i = 0; i < touch_event->touches_length; ++i) {
      if (ResampleSinglePointer(frame_time, &touch_event->touches[i]))
        event->SetTimeStamp(frame_time);
    }
  } else if (event->GetType() == WebInputEvent::kMouseMove) {
    if (ResampleSinglePointer(frame_time, static_cast<WebMouseEvent*>(event)))
      event->SetTimeStamp(frame_time);
  } else if (event->GetType() == WebInputEvent::kPointerMove) {
    if (ResampleSinglePointer(frame_time, static_cast<WebPointerEvent*>(event)))
      event->SetTimeStamp(frame_time);
  }
}

void InputEventPrediction::ResetPredictor(const WebInputEvent& event) {
  if (WebInputEvent::IsTouchEventType(event.GetType())) {
    const WebTouchEvent& touch_event = static_cast<const WebTouchEvent&>(event);
    for (unsigned i = 0; i < touch_event.touches_length; ++i) {
      if (touch_event.touches[i].state != blink::WebTouchPoint::kStateMoved &&
          touch_event.touches[i].state !=
              blink::WebTouchPoint::kStateStationary)
        pointer_id_predictor_map_.erase(touch_event.touches[i].id);
    }
  } else if (WebInputEvent::IsMouseEventType(event.GetType())) {
    ResetSinglePredictor(static_cast<const WebMouseEvent&>(event));
  } else if (WebInputEvent::IsPointerEventType(event.GetType())) {
    ResetSinglePredictor(static_cast<const WebPointerEvent&>(event));
  }
}

void InputEventPrediction::UpdateSinglePointer(
    const WebPointerProperties& event,
    base::TimeTicks event_time) {
  ui::InputPredictor::InputData data = {event.PositionInWidget().x,
                                        event.PositionInWidget().y, event_time};
  if (event.pointer_type == WebPointerProperties::PointerType::kMouse)
    mouse_predictor_->Update(data);
  else {
    auto predictor = pointer_id_predictor_map_.find(event.id);
    if (predictor != pointer_id_predictor_map_.end()) {
      predictor->second->Update(data);
    } else {
      pointer_id_predictor_map_.insert({event.id, SetUpPredictor()});
      pointer_id_predictor_map_[event.id]->Update(data);
    }
  }
}

bool InputEventPrediction::ResampleSinglePointer(base::TimeTicks frame_time,
                                                 WebPointerProperties* event) {
  ui::InputPredictor::InputData predict_result;
  if (event->pointer_type == WebPointerProperties::PointerType::kMouse) {
    if (mouse_predictor_->HasPrediction() &&
        mouse_predictor_->GeneratePrediction(frame_time, &predict_result)) {
      event->SetPositionInWidget(predict_result.pos_x, predict_result.pos_y);
      return true;
    }
  } else {
    // Reset mouse predictor if pointer type is touch or stylus
    mouse_predictor_->Reset();

    auto predictor = pointer_id_predictor_map_.find(event->id);
    if (predictor != pointer_id_predictor_map_.end() &&
        predictor->second->HasPrediction() &&
        predictor->second->GeneratePrediction(frame_time, &predict_result)) {
      event->SetPositionInWidget(predict_result.pos_x, predict_result.pos_y);
      return true;
    }
  }
  return false;
}

void InputEventPrediction::ResetSinglePredictor(
    const WebPointerProperties& event) {
  if (event.pointer_type == WebPointerProperties::PointerType::kMouse)
    mouse_predictor_->Reset();
  else
    pointer_id_predictor_map_.erase(event.id);
}

}  // namespace content
