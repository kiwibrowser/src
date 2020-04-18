// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/interfaces/constants.mojom.h"
#include "ash/public/interfaces/time_to_first_present_recorder_test_api.mojom.h"
#include "base/command_line.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

using TimeToFirstPresentRecorderTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(TimeToFirstPresentRecorderTest, VerifyTimeCalculated) {
  ash::mojom::TimeToFirstPresentRecorderTestApiPtr recorder_test_api;
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &recorder_test_api);
  ash::mojom::TimeToFirstPresentRecorderTestApiAsyncWaiter recorder(
      recorder_test_api.get());
  base::TimeDelta time_delta;
  recorder.GetProcessCreationToFirstPresentTime(&time_delta);
  EXPECT_FALSE(time_delta.is_zero());
}
