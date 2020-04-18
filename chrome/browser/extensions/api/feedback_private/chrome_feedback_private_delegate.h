// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_FEEDBACK_PRIVATE_CHROME_FEEDBACK_PRIVATE_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_API_FEEDBACK_PRIVATE_CHROME_FEEDBACK_PRIVATE_DELEGATE_H_

#include "extensions/browser/api/feedback_private/feedback_private_delegate.h"

#include "base/macros.h"

namespace extensions {

class ChromeFeedbackPrivateDelegate : public FeedbackPrivateDelegate {
 public:
  ChromeFeedbackPrivateDelegate();
  ~ChromeFeedbackPrivateDelegate() override;

  // FeedbackPrivateDelegate:
  std::unique_ptr<base::DictionaryValue> GetStrings(
      content::BrowserContext* browser_context,
      bool from_crash) const override;
  system_logs::SystemLogsFetcher* CreateSystemLogsFetcher(
      content::BrowserContext* context) const override;
#if defined(OS_CHROMEOS)
  std::unique_ptr<system_logs::SystemLogsSource> CreateSingleLogSource(
      api::feedback_private::LogSource source_type) const override;
#endif  // defined(OS_CHROMEOS)
  std::string GetSignedInUserEmail(
      content::BrowserContext* context) const override;
  void NotifyFeedbackDelayed() const override;
  feedback::FeedbackUploader* GetFeedbackUploaderForContext(
      content::BrowserContext* context) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeFeedbackPrivateDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_FEEDBACK_PRIVATE_CHROME_FEEDBACK_PRIVATE_DELEGATE_H_
