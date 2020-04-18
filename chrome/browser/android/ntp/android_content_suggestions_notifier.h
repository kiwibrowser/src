// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_NTP_ANDROID_CONTENT_SUGGESTIONS_NOTIFIER_H_
#define CHROME_BROWSER_ANDROID_NTP_ANDROID_CONTENT_SUGGESTIONS_NOTIFIER_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "chrome/browser/android/ntp/content_suggestions_notifier.h"
#include "chrome/browser/ntp_snippets/ntp_snippets_metrics.h"
#include "url/gurl.h"

// Implements ContentSuggestionsNotifier methods with JNI calls.
class AndroidContentSuggestionsNotifier : public ContentSuggestionsNotifier {
 public:
  AndroidContentSuggestionsNotifier();

  // ContentSuggestionsNotifier
  bool SendNotification(const ntp_snippets::ContentSuggestion::ID& id,
                        const GURL& url,
                        const base::string16& title,
                        const base::string16& text,
                        const gfx::Image& image,
                        base::Time timeout_at,
                        int priority) override;
  void HideNotification(const ntp_snippets::ContentSuggestion::ID& id,
                        ContentSuggestionsNotificationAction why) override;
  void HideAllNotifications(ContentSuggestionsNotificationAction why) override;
  void FlushCachedMetrics() override;
  bool RegisterChannel(bool enabled) override;
  void UnregisterChannel() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AndroidContentSuggestionsNotifier);
};

#endif  // CHROME_BROWSER_ANDROID_NTP_ANDROID_CONTENT_SUGGESTIONS_NOTIFIER_H_
