// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_INPUT_EVENT_PREDICTION_H_
#define CONTENT_RENDERER_INPUT_INPUT_EVENT_PREDICTION_H_

#include <list>
#include <unordered_map>

#include "content/renderer/input/scoped_web_input_event_with_latency_info.h"
#include "ui/events/blink/prediction/input_predictor.h"
#include "ui/events/event.h"

using blink::WebInputEvent;
using blink::WebPointerProperties;

namespace content {

// Handle resampling of WebMouseEvent, WebTouchEvent and WebPointerEvent.
// This class stores prediction of all active pointers.
class CONTENT_EXPORT InputEventPrediction {
 public:
  InputEventPrediction();
  ~InputEventPrediction();

  void HandleEvents(const blink::WebCoalescedInputEvent& coalesced_event,
                    base::TimeTicks frame_time,
                    blink::WebInputEvent* event);

 private:
  // The following three function is for handling multiple TouchPoints in a
  // WebTouchEvent. They should be more neat when WebTouchEvent is elimated.
  // Cast events from WebInputEvent to WebPointerProperties. Call
  // UpdateSinglePointer for each pointer.
  void UpdatePrediction(const WebInputEvent& event);
  // Cast events from WebInputEvent to WebPointerProperties. Call
  // ResamplingSinglePointer for each poitner.
  void ApplyResampling(base::TimeTicks frame_time, WebInputEvent* event);
  // Cast events from WebInputEvent to WebPointerProperties. Call
  // ResetSinglePredictor for each pointer.
  void ResetPredictor(const WebInputEvent& event);

  // Get single predictor based on event id and type, and update the predictor
  // with new events coords.
  void UpdateSinglePointer(const WebPointerProperties& event,
                           base::TimeTicks time);
  // Get single predictor based on event id and type, apply resampling event
  // coordinates.
  bool ResampleSinglePointer(base::TimeTicks time, WebPointerProperties* event);
  // Get single predictor based on event id and type. For mouse, reset the
  // predictor, for other pointer type, remove it from mapping.
  void ResetSinglePredictor(const WebPointerProperties& event);

  friend class InputEventPredictionTest;

  std::unordered_map<ui::PointerId, std::unique_ptr<ui::InputPredictor>>
      pointer_id_predictor_map_;
  std::unique_ptr<ui::InputPredictor> mouse_predictor_;

  DISALLOW_COPY_AND_ASSIGN(InputEventPrediction);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_INPUT_EVENT_PREDICTION_H_
