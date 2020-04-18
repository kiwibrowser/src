// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_METRICS_TIME_TO_FIRST_PRESENT_RECORDER_H_
#define ASH_METRICS_TIME_TO_FIRST_PRESENT_RECORDER_H_

#include <stdint.h>

#include "ash/public/interfaces/process_creation_time_recorder.mojom.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace aura {
class Window;
}

namespace ash {

class TimeToFirstPresentRecorderTestApi;

// Used for tracking the time main started to the time the first bits make it
// the screen and logging a histogram of the time. Chrome is responsible for
// providing the start time by way of ProcessCreationTimeRecorder.
//
// This only logs the time to present the primary root window.
class TimeToFirstPresentRecorder : public mojom::ProcessCreationTimeRecorder {
 public:
  explicit TimeToFirstPresentRecorder(aura::Window* window);
  ~TimeToFirstPresentRecorder() override;

  void Bind(mojom::ProcessCreationTimeRecorderRequest request);

 private:
  friend class TimeToFirstPresentRecorderTestApi;

  // If both times are available the time to present is logged.
  void LogTime();

  // Callback from the compositor when it presented a valid frame.
  void DidPresentCompositorFrame(base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags);

  base::TimeDelta time_to_first_present() const {
    return present_time_ - process_creation_time_;
  }

  // mojom::ProcessCreationTimeRecorder:
  void SetMainProcessCreationTime(base::TimeTicks start_time) override;

  base::TimeTicks process_creation_time_;
  base::TimeTicks present_time_;

  // Only used by tests. If valid it's Run() when both times are determined.
  base::OnceClosure log_callback_;

  mojo::Binding<mojom::ProcessCreationTimeRecorder> binding_{this};

  DISALLOW_COPY_AND_ASSIGN(TimeToFirstPresentRecorder);
};

}  // namespace ash

#endif  // ASH_METRICS_TIME_TO_FIRST_PRESENT_RECORDER_H_
