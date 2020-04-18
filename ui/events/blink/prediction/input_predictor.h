// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_BLINK_PREDICTION_INPUT_PREDICTOR_H_
#define UI_EVENTS_BLINK_PREDICTION_INPUT_PREDICTOR_H_

#include <memory>

#include "base/macros.h"
#include "ui/events/base_event_utils.h"

namespace ui {

// This class expects a sequence of inputs with their coordinates and timestamps
// and models the input path. It then can predict the coordinates at any given
// time.
class InputPredictor {
 public:
  virtual ~InputPredictor() = default;

  struct InputData {
    double pos_x;
    double pos_y;
    base::TimeTicks time_stamp;
  };

  // Reset should be called each time when a new line start.
  virtual void Reset() = 0;

  // Update the predictor with new input points.
  virtual void Update(const InputData& new_input) = 0;

  // Return true if the predictor is able to predict points.
  virtual bool HasPrediction() const = 0;

  // Generate the prediction based on current points.
  virtual bool GeneratePrediction(base::TimeTicks frame_time,
                                  InputData* result) const = 0;
};

}  // namespace ui

#endif  // UI_EVENTS_BLINK_PREDICTION_INPUT_PREDICTOR_H_
