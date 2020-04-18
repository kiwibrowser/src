// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/fake_app_remoting_report_issue_request.h"

#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"

namespace remoting {
namespace test {

std::string MakeFormattedStringForReleasedHost(
    const std::string& application_id,
    const std::string& host_id) {
  return application_id + "::" + host_id;
}

FakeAppRemotingReportIssueRequest::FakeAppRemotingReportIssueRequest()
    : fail_start_request_(false) {
}

FakeAppRemotingReportIssueRequest::~FakeAppRemotingReportIssueRequest() =
    default;

bool FakeAppRemotingReportIssueRequest::Start(
    const std::string& application_id,
    const std::string& host_id,
    const std::string& access_token,
    ServiceEnvironment service_environment,
    bool abandon_host,
    base::Closure done_callback) {
  if (fail_start_request_) {
    done_callback.Run();
    return false;
  }

  if (abandon_host) {
    std::string host_id_string(application_id + "::" + host_id);
    host_ids_released_.push_back(MakeFormattedStringForReleasedHost(
      application_id, host_id));
  }

  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      base::ThreadTaskRunnerHandle::Get();
  task_runner->PostTask(FROM_HERE, done_callback);

  return true;
}

}  // namespace test
}  // namespace remoting
