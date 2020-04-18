// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_FAKE_APP_REMOTING_REPORT_ISSUE_REQUEST_H_
#define REMOTING_TEST_FAKE_APP_REMOTING_REPORT_ISSUE_REQUEST_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "remoting/test/app_remoting_report_issue_request.h"

namespace remoting {
namespace test {

// Generates a string used to track the 'released' host id by the
// FakeAppRemotingReportIssueRequest class.
std::string MakeFormattedStringForReleasedHost(
    const std::string& application_id,
    const std::string& host_id);

// Used for testing classes which rely on the AccessTokenFetcher and want to
// simulate success and failure scenarios without using the actual class and
// network connection.
class FakeAppRemotingReportIssueRequest : public AppRemotingReportIssueRequest {
 public:
  FakeAppRemotingReportIssueRequest();
  ~FakeAppRemotingReportIssueRequest() override;

  // AppRemotingReportIssueRequest interface.
  bool Start(const std::string& application_id,
             const std::string& host_id,
             const std::string& access_token,
             ServiceEnvironment service_environment,
             bool abandon_host,
             base::Closure done_callback) override;

  void set_fail_start_request(bool fail) { fail_start_request_ = fail; }

  const std::vector<std::string>& get_host_ids_released() {
    return host_ids_released_;
  }

 private:
  // True if Start() should fail.
  bool fail_start_request_;

  // Contains the set of host ids which have been released, the string contained
  // will be in the form "<application_id>::<host_id>";
  std::vector<std::string> host_ids_released_;

  DISALLOW_COPY_AND_ASSIGN(FakeAppRemotingReportIssueRequest);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_FAKE_APP_REMOTING_REPORT_ISSUE_REQUEST_H_
