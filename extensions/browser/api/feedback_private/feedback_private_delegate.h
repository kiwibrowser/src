// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_FEEDBACK_PRIVATE_DELEGATE_H_
#define EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_FEEDBACK_PRIVATE_DELEGATE_H_

#include "extensions/common/api/feedback_private.h"

#include <memory>
#include <string>

namespace base {
class DictionaryValue;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace feedback {
class FeedbackUploader;
}  // namespace feedback

namespace system_logs {
class SystemLogsFetcher;
class SystemLogsSource;
}  // namespace system_logs

namespace extensions {

// Delegate class for embedder-specific chrome.feedbackPrivate behavior.
class FeedbackPrivateDelegate {
 public:
  virtual ~FeedbackPrivateDelegate() = default;

  // Returns a dictionary of localized strings for the feedback component
  // extension.
  // Set |from_crash| to customize strings when the feedback UI was initiated
  // from a "sad tab" crash.
  virtual std::unique_ptr<base::DictionaryValue> GetStrings(
      content::BrowserContext* browser_context,
      bool from_crash) const = 0;

  // Returns a SystemLogsFetcher for responding to a request for system logs.
  virtual system_logs::SystemLogsFetcher* CreateSystemLogsFetcher(
      content::BrowserContext* context) const = 0;

#if defined(OS_CHROMEOS)
  // Creates a SystemLogsSource for the given type of log file.
  virtual std::unique_ptr<system_logs::SystemLogsSource> CreateSingleLogSource(
      api::feedback_private::LogSource source_type) const = 0;
#endif

  // Returns the normalized email address of the signed-in user associated with
  // the browser context, if any.
  virtual std::string GetSignedInUserEmail(
      content::BrowserContext* context) const = 0;

  // Called if sending the feedback report was delayed.
  virtual void NotifyFeedbackDelayed() const = 0;

  // Returns the uploader associated with |context| which is used to upload
  // feedback reports to the feedback server.
  virtual feedback::FeedbackUploader* GetFeedbackUploaderForContext(
      content::BrowserContext* context) const = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_FEEDBACK_PRIVATE_DELEGATE_H_
