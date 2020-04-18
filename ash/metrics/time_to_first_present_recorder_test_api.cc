// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/metrics/time_to_first_present_recorder_test_api.h"

#include "ash/metrics/time_to_first_present_recorder.h"
#include "ash/shell.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace ash {

TimeToFirstPresentRecorderTestApi::TimeToFirstPresentRecorderTestApi() =
    default;

TimeToFirstPresentRecorderTestApi::~TimeToFirstPresentRecorderTestApi() =
    default;

// static
void TimeToFirstPresentRecorderTestApi::BindRequest(
    mojom::TimeToFirstPresentRecorderTestApiRequest request) {
  mojo::MakeStrongBinding(std::make_unique<TimeToFirstPresentRecorderTestApi>(),
                          std::move(request));
}

void TimeToFirstPresentRecorderTestApi::GetProcessCreationToFirstPresentTime(
    GetProcessCreationToFirstPresentTimeCallback callback) {
  TimeToFirstPresentRecorder* recorder =
      Shell::Get()->time_to_first_present_recorder();
  if (recorder->process_creation_time_.is_null() ||
      recorder->present_time_.is_null()) {
    // Still waiting for time. Schedule a callback with
    // TimeToFirstPresentRecorder. This only supports one callback at a time,
    // which should be fine for tests.
    DCHECK(recorder->log_callback_.is_null());
    recorder->log_callback_ = base::BindOnce(
        &TimeToFirstPresentRecorderTestApi::OnLog, base::Unretained(this));
    DCHECK(!get_creation_time_callback_);
    get_creation_time_callback_ = std::move(callback);
    return;
  }
  std::move(callback).Run(recorder->time_to_first_present());
}

void TimeToFirstPresentRecorderTestApi::OnLog() {
  TimeToFirstPresentRecorder* recorder =
      Shell::Get()->time_to_first_present_recorder();
  DCHECK(!recorder->process_creation_time_.is_null() &&
         !recorder->present_time_.is_null());
  std::move(get_creation_time_callback_)
      .Run(Shell::Get()
               ->time_to_first_present_recorder()
               ->time_to_first_present());
}

}  // namespace ash
