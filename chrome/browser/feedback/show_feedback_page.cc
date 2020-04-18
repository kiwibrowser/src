// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/feedback/feedback_dialog_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "extensions/browser/api/feedback_private/feedback_private_api.h"

namespace feedback_private = extensions::api::feedback_private;

namespace chrome {

void ShowFeedbackPage(Browser* browser,
                      FeedbackSource source,
                      const std::string& description_template,
                      const std::string& description_placeholder_text,
                      const std::string& category_tag,
                      const std::string& extra_diagnostics) {
  GURL page_url;
  if (browser) {
    page_url = GetTargetTabUrl(browser->session_id(),
                               browser->tab_strip_model()->active_index());
  }

  Profile* profile = GetFeedbackProfile(browser);
  if (!profile) {
    LOG(ERROR) << "Cannot invoke feedback: No profile found!";
    return;
  }

  // Record an UMA histogram to know the most frequent feedback request source.
  UMA_HISTOGRAM_ENUMERATION("Feedback.RequestSource", source,
                            kFeedbackSourceCount);

  extensions::FeedbackPrivateAPI* api =
      extensions::FeedbackPrivateAPI::GetFactoryInstance()->Get(profile);

  api->RequestFeedbackForFlow(
      description_template, description_placeholder_text, category_tag,
      extra_diagnostics, page_url,
      source == kFeedbackSourceSadTabPage
          ? feedback_private::FeedbackFlow::FEEDBACK_FLOW_SADTABCRASH
          : feedback_private::FeedbackFlow::FEEDBACK_FLOW_REGULAR);
}

}  // namespace chrome
