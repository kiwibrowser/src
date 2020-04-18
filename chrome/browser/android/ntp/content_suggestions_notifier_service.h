// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_NTP_CONTENT_SUGGESTIONS_NOTIFIER_SERVICE_H_
#define CHROME_BROWSER_ANDROID_NTP_CONTENT_SUGGESTIONS_NOTIFIER_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

class ContentSuggestionsNotifier;
class PrefService;

namespace ntp_snippets {
class ContentSuggestionsService;
}  // namespace ntp_snippets

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

class ContentSuggestionsNotifierService : public KeyedService {
 public:
  ContentSuggestionsNotifierService(
      PrefService* prefs,
      ntp_snippets::ContentSuggestionsService* suggestions,
      std::unique_ptr<ContentSuggestionsNotifier> notifier);

  ~ContentSuggestionsNotifierService() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Set whether the service is allowed to show notifications.
  void SetEnabled(bool enabled);

  // Returns whether the service is allowed to show notifications.
  bool IsEnabled() const;

 private:
  class NotifyingObserver;

  // Creates |observer_| if necessary and registers notification channel.
  void Enable();

  // Destroys |observer_| if necessary and deregisters notification channel.
  void Disable();

  PrefService* const prefs_;
  ntp_snippets::ContentSuggestionsService* const suggestions_service_;
  const std::unique_ptr<ContentSuggestionsNotifier> notifier_;

  std::unique_ptr<NotifyingObserver> observer_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(ContentSuggestionsNotifierService);
};

#endif  // CHROME_BROWSER_ANDROID_NTP_CONTENT_SUGGESTIONS_NOTIFIER_SERVICE_H_
