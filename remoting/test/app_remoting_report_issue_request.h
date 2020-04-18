// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_APP_REMOTING_REPORT_ISSUE_REQUEST_H_
#define REMOTING_TEST_APP_REMOTING_REPORT_ISSUE_REQUEST_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "remoting/test/app_remoting_service_urls.h"

namespace remoting {
class URLRequestContextGetter;
}

namespace remoting {
namespace test {

// Calls the App Remoting service API to report an issue.  This is typically
// used to abandon a remote host or to upload crash logs.
// Must be used from a thread running an IO message loop.
// The public method is virtual to allow for testing using a fake.
class AppRemotingReportIssueRequest : public net::URLFetcherDelegate {
 public:
  AppRemotingReportIssueRequest();
  ~AppRemotingReportIssueRequest() override;

  // Makes a service call to the ReportIssue API.
  virtual bool Start(const std::string& application_id,
                     const std::string& host_id,
                     const std::string& access_token,
                     ServiceEnvironment service_environment,
                     bool abandon_host,
                     base::Closure done_callback);

 private:
  // net::URLFetcherDelegate interface.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Holds the URLFetcher for the ReportIssue request.
  std::unique_ptr<net::URLFetcher> request_;

  // Provides application-specific context for the network request.
  scoped_refptr<remoting::URLRequestContextGetter> request_context_getter_;

  // Caller-supplied callback which is signalled when the request is complete.
  base::Closure request_complete_callback_;

  DISALLOW_COPY_AND_ASSIGN(AppRemotingReportIssueRequest);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_APP_REMOTING_REPORT_ISSUE_REQUEST_H_
