// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_METRICS_TIME_TO_FIRST_PRESENT_RECORDER_TEST_API_H_
#define ASH_METRICS_TIME_TO_FIRST_PRESENT_RECORDER_TEST_API_H_

#include "ash/public/interfaces/time_to_first_present_recorder_test_api.mojom.h"
#include "base/macros.h"

namespace ash {

class TimeToFirstPresentRecorderTestApi
    : public mojom::TimeToFirstPresentRecorderTestApi {
 public:
  TimeToFirstPresentRecorderTestApi();
  ~TimeToFirstPresentRecorderTestApi() override;

  // Creates and binds an instance from a remote request (e.g. from chrome).
  static void BindRequest(
      mojom::TimeToFirstPresentRecorderTestApiRequest request);

  // mojom::TimeToFirstPresentRecorderTestApi:
  void GetProcessCreationToFirstPresentTime(
      GetProcessCreationToFirstPresentTimeCallback callback) override;

 private:
  void OnLog();

  // If valid GetProcessCreationToFirstPresentTimeCallback() was called and
  // we're waiting for TimeToFirstPresentRecorder to see the first log.
  GetProcessCreationToFirstPresentTimeCallback get_creation_time_callback_;

  DISALLOW_COPY_AND_ASSIGN(TimeToFirstPresentRecorderTestApi);
};

}  // namespace ash

#endif  // ASH_METRICS_TIME_TO_FIRST_PRESENT_RECORDER_TEST_API_H_
