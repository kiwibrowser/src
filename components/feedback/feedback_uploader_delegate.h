// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_DELEGATE_H_
#define COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_DELEGATE_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/feedback/feedback_uploader.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace feedback {

// Type of the callback that gets invoked when uploading a feedback report
// fails. |should_retry| is set to true when it's OK to retry sending the
// report; e.g. when the failure is not a client error and retries is likely to
// fail again.
using ReportFailureCallback = base::Callback<void(bool should_retry)>;

// FeedbackUploaderDelegate is a simple HTTP uploader for a feedback report.
// When finished, it runs the appropriate callback passed in via the
// constructor, and then deletes itself.
class FeedbackUploaderDelegate : public net::URLFetcherDelegate {
 public:
  FeedbackUploaderDelegate(const base::Closure& success_callback,
                           const ReportFailureCallback& error_callback);
  ~FeedbackUploaderDelegate() override;

 private:
  // Overridden from net::URLFetcherDelegate.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  base::Closure success_callback_;
  ReportFailureCallback error_callback_;

  DISALLOW_COPY_AND_ASSIGN(FeedbackUploaderDelegate);
};

}  // namespace feedback

#endif  // COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_DELEGATE_H_
