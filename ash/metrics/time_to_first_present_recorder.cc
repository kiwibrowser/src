// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/metrics/time_to_first_present_recorder.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/compositor/compositor.h"

namespace ash {

TimeToFirstPresentRecorder::TimeToFirstPresentRecorder(aura::Window* window) {
  aura::WindowTreeHost* window_tree_host = window->GetHost();
  DCHECK(window_tree_host);
  window_tree_host->compositor()->RequestPresentationTimeForNextFrame(
      base::BindOnce(&TimeToFirstPresentRecorder::DidPresentCompositorFrame,
                     base::Unretained(this)));
}

TimeToFirstPresentRecorder::~TimeToFirstPresentRecorder() = default;

void TimeToFirstPresentRecorder::Bind(
    mojom::ProcessCreationTimeRecorderRequest request) {
  // Process createion time should only be set once.
  if (binding_.is_bound() || !process_creation_time_.is_null())
    return;

  binding_.Bind(std::move(request));
}

void TimeToFirstPresentRecorder::SetMainProcessCreationTime(
    base::TimeTicks start_time) {
  if (!process_creation_time_.is_null())
    return;

  process_creation_time_ = start_time;
  LogTime();

  // Process creation time should be set only once.
  binding_.Close();
}

void TimeToFirstPresentRecorder::LogTime() {
  if (present_time_.is_null() || process_creation_time_.is_null())
    return;

  UMA_HISTOGRAM_TIMES("Ash.ProcessCreationToFirstPresent",
                      time_to_first_present());
  if (log_callback_)
    std::move(log_callback_).Run();
}

void TimeToFirstPresentRecorder::DidPresentCompositorFrame(
    base::TimeTicks time,
    base::TimeDelta refresh,
    uint32_t flags) {
  DCHECK(present_time_.is_null());  // This should only be called once.
  present_time_ = time;
  LogTime();
}

}  // namespace ash
