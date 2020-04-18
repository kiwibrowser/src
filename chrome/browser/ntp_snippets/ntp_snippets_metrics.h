// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NTP_SNIPPETS_NTP_SNIPPETS_METRICS_H_
#define CHROME_BROWSER_NTP_SNIPPETS_NTP_SNIPPETS_METRICS_H_

enum class ContentSuggestionsNotificationImpression {
  ARTICLE = 0,  // Server-provided "articles" category.
  NONARTICLE,   // Anything else.

  MAX
};

// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.ntp.snippets
enum class ContentSuggestionsNotificationAction {
  TAP = 0,    // User tapped notification to open article.
  DISMISSAL,  // User swiped notification to dismiss it.

  HIDE_DEADLINE,   // notification_extra().deadline passed.
  HIDE_EXPIRY,     // NTP no longer shows notified article.
  HIDE_FRONTMOST,  // Chrome became the frontmost app.
  HIDE_DISABLED,   // NTP no longer shows whole category.
  HIDE_SHUTDOWN,   // Content sugg service is shutting down.

  OPEN_SETTINGS,  // User opened settings from notification.

  MAX
};

// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.ntp.snippets
enum class ContentSuggestionsNotificationOptOut {
  IMPLICIT = 0,  // User ignored notifications.
  EXPLICIT,      // User explicitly opted-out.

  MAX
};

void RecordContentSuggestionsNotificationImpression(
    ContentSuggestionsNotificationImpression what);
void RecordContentSuggestionsNotificationAction(
    ContentSuggestionsNotificationAction what);
void RecordContentSuggestionsNotificationOptOut(
    ContentSuggestionsNotificationOptOut what);

#endif  // CHROME_BROWSER_NTP_SNIPPETS_NTP_SNIPPETS_METRICS_H_
