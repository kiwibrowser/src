// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_TEST_MOCK_CONTROLLER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_TEST_MOCK_CONTROLLER_H_

#include "base/macros.h"
#include "components/download/internal/background_service/controller.h"
#include "components/download/internal/background_service/startup_status.h"
#include "components/download/public/background_service/download_params.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace download {
namespace test {

class MockController : public Controller {
 public:
  MockController();
  ~MockController() override;

  // Controller implementation.
  void Initialize(const base::Closure& callback) override;
  MOCK_METHOD0(GetState, Controller::State());
  MOCK_METHOD1(StartDownload, void(const DownloadParams&));
  MOCK_METHOD1(PauseDownload, void(const std::string&));
  MOCK_METHOD1(ResumeDownload, void(const std::string&));
  MOCK_METHOD1(CancelDownload, void(const std::string&));
  MOCK_METHOD2(ChangeDownloadCriteria,
               void(const std::string&, const SchedulingParams&));
  MOCK_METHOD1(GetOwnerOfDownload, DownloadClient(const std::string&));
  MOCK_METHOD2(OnStartScheduledTask,
               void(DownloadTaskType, const TaskFinishedCallback&));
  MOCK_METHOD1(OnStopScheduledTask, bool(DownloadTaskType task_type));

  void TriggerInitCompleted();

 private:
  base::Closure init_callback_;
  DISALLOW_COPY_AND_ASSIGN(MockController);
};

}  // namespace test
}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_TEST_MOCK_CONTROLLER_H_
