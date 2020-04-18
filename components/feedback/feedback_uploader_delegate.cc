// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feedback/feedback_uploader_delegate.h"

#include <memory>
#include <sstream>

#include "base/logging.h"
#include "components/feedback/feedback_report.h"
#include "net/url_request/url_fetcher.h"

namespace feedback {

namespace {

constexpr int kHttpPostSuccessNoContent = 204;
constexpr int kHttpPostFailNoConnection = -1;
constexpr int kHttpPostFailClientError = 400;
constexpr int kHttpPostFailServerError = 500;

}  // namespace

FeedbackUploaderDelegate::FeedbackUploaderDelegate(
    const base::Closure& success_callback,
    const ReportFailureCallback& error_callback)
    : success_callback_(success_callback), error_callback_(error_callback) {}

FeedbackUploaderDelegate::~FeedbackUploaderDelegate() {}

void FeedbackUploaderDelegate::OnURLFetchComplete(
    const net::URLFetcher* source) {
  std::unique_ptr<const net::URLFetcher> source_scoper(source);

  std::stringstream error_stream;
  int response_code = source->GetResponseCode();
  if (response_code == kHttpPostSuccessNoContent) {
    error_stream << "Success";
    success_callback_.Run();
  } else {
    bool should_retry = true;
    // Process the error for debug output
    if (response_code == kHttpPostFailNoConnection) {
      error_stream << "No connection to server.";
    } else if ((response_code >= kHttpPostFailClientError) &&
               (response_code < kHttpPostFailServerError)) {
      // Client errors mean that the server failed to parse the proto that was
      // sent, or that some requirements weren't met by the server side
      // validation, and hence we should NOT retry sending this corrupt report
      // and give up.
      should_retry = false;

      error_stream << "Client error: HTTP response code " << response_code;
    } else if (response_code >= kHttpPostFailServerError) {
      error_stream << "Server error: HTTP response code " << response_code;
    } else {
      error_stream << "Unknown error: HTTP response code " << response_code;
    }

    error_callback_.Run(should_retry);
  }

  LOG(WARNING) << "FEEDBACK: Submission to feedback server ("
               << source->GetURL() << ") status: " << error_stream.str();

  // This instance won't be used for anything else, delete us.
  delete this;
}

}  // namespace feedback
