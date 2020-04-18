// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_NTP_CONTENT_SUGGESTIONS_NOTIFIER_H_
#define CHROME_BROWSER_ANDROID_NTP_CONTENT_SUGGESTIONS_NOTIFIER_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "chrome/browser/ntp_snippets/ntp_snippets_metrics.h"
#include "components/ntp_snippets/content_suggestion.h"

class GURL;
class PrefService;

namespace gfx {
class Image;
}  // namespace gfx

class ContentSuggestionsNotifier {
 public:
  ContentSuggestionsNotifier();
  virtual ~ContentSuggestionsNotifier();

  // Returns true if notifications should be sent.
  //
  // This function considers:
  //   * If the user has disabled notifications through preferences.
  //   * If the user has ignored enough consecutive notifications to treat that
  //     as disabling notifications (if auto-opt-out is enabled).
  //
  // It does not consider:
  //   * Whether the notifications feature is enabled. In this case, none of the
  //     notifications machinery is instantiated to begin with.
  //   * On Android O, if the notification channel is disabled.
  static bool ShouldSendNotifications(PrefService* prefs);

  virtual bool SendNotification(const ntp_snippets::ContentSuggestion::ID& id,
                                const GURL& url,
                                const base::string16& title,
                                const base::string16& text,
                                const gfx::Image& image,
                                base::Time timeout_at,
                                int priority) = 0;
  virtual void HideNotification(const ntp_snippets::ContentSuggestion::ID& id,
                                ContentSuggestionsNotificationAction why) = 0;
  virtual void HideAllNotifications(
      ContentSuggestionsNotificationAction why) = 0;

  // Moves metrics tracked in Java into native histograms. Should be called when
  // the native library starts up, to capture any actions that were taken since
  // the last time it was running. (Harmless to call more often, though)
  //
  // Also updates the "consecutive ignored" preference, which is computed from
  // the actions taken on notifications, and maybe the "opt outs" metric, which
  // is computed in turn from that.
  virtual void FlushCachedMetrics() = 0;

  // Registers the notification channel on Android O. May be called regardless
  // of Android version or registration state; it is a no-op before Android O,
  // or if the channel is already registered. If |!enabled|, then the channel is
  // disabled at creation. Returns true if the channel was created (false pre-O,
  // or if it already existed).
  virtual bool RegisterChannel(bool enabled) = 0;

  // Unregisters the notification channel on Android O. May be called regardless
  // of Android version or registration state; it is a no-op before Android O,
  // or if the channel is not registered.
  virtual void UnregisterChannel() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContentSuggestionsNotifier);
};

#endif  // CHROME_BROWSER_ANDROID_NTP_CONTENT_SUGGESTIONS_NOTIFIER_H_
