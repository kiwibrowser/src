// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/ntp/get_remote_suggestions_scheduler.h"

#include "chrome/browser/ntp_snippets/content_suggestions_service_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/ntp_snippets/content_suggestions_service.h"

ntp_snippets::RemoteSuggestionsScheduler* GetRemoteSuggestionsScheduler() {
  ntp_snippets::ContentSuggestionsService* content_suggestions_service =
      ContentSuggestionsServiceFactory::GetForProfile(
          ProfileManager::GetLastUsedProfile());
  // Can maybe be null in some cases? (Incognito profile?) crbug.com/647920
  if (!content_suggestions_service) {
    return nullptr;
  }
  return content_suggestions_service->remote_suggestions_scheduler();
}
