// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/app_remoting_report_issue_request.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_fetcher.h"
#include "remoting/base/url_request_context_getter.h"

namespace {
const char kRequestTestOrigin[] =
    "Origin: chrome-extension://ljacajndfccfgnfohlgkdphmbnpkjflk";
}

namespace remoting {
namespace test {

AppRemotingReportIssueRequest::AppRemotingReportIssueRequest() = default;

AppRemotingReportIssueRequest::~AppRemotingReportIssueRequest() = default;

bool AppRemotingReportIssueRequest::Start(
    const std::string& application_id,
    const std::string& host_id,
    const std::string& access_token,
    ServiceEnvironment service_environment,
    bool abandon_host,
    base::Closure done_callback) {
  DCHECK(request_complete_callback_.is_null()) << "Request pending";

  VLOG(2) << "AppRemotingReportIssueRequest::Start() called";

  std::string service_url(
      GetReportIssueUrl(application_id, host_id, service_environment));
  if (service_url.empty()) {
    LOG(ERROR) << "Unrecognized service type: " << service_environment;
    return false;
  }
  VLOG(1) << "Sending Report Issue service request to: " << service_url;

  request_complete_callback_ = done_callback;

  request_context_getter_ = new remoting::URLRequestContextGetter(
      base::ThreadTaskRunnerHandle::Get());

  std::string upload_data = abandon_host ? "{ 'abandonHost': 'true' }" : "{}";

  request_ = net::URLFetcher::Create(GURL(service_url), net::URLFetcher::POST,
                                     this, TRAFFIC_ANNOTATION_FOR_TESTS);
  request_->SetRequestContext(request_context_getter_.get());
  request_->AddExtraRequestHeader("Authorization: OAuth " + access_token);
  request_->AddExtraRequestHeader(kRequestTestOrigin);
  request_->SetUploadData("application/json; charset=UTF-8", upload_data);
  request_->Start();

  return true;
}

void AppRemotingReportIssueRequest::OnURLFetchComplete(
    const net::URLFetcher* source) {
  VLOG(2) << "URL Fetch Completed for: " << source->GetOriginalURL();

  int response_code = request_->GetResponseCode();
  if (response_code != net::HTTP_OK && response_code != net::HTTP_NO_CONTENT) {
    LOG(ERROR) << "ReportIssue request failed with error code: "
               << response_code;
  }

  base::ResetAndReturn(&request_complete_callback_).Run();
}

}  // namespace test
}  // namespace remoting
