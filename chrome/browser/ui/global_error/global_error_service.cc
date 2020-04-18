// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/global_error/global_error_service.h"

#include <stddef.h>

#include <algorithm>

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/global_error/global_error.h"
#include "chrome/browser/ui/global_error/global_error_bubble_view_base.h"
#include "content/public/browser/notification_service.h"

GlobalErrorService::GlobalErrorService(Profile* profile) : profile_(profile) {
}

GlobalErrorService::~GlobalErrorService() {}

void GlobalErrorService::AddGlobalError(std::unique_ptr<GlobalError> error) {
  DCHECK(error);
  GlobalError* error_ptr = error.get();
  owned_errors_[error_ptr] = std::move(error);
  AddUnownedGlobalError(error_ptr);
}

void GlobalErrorService::AddUnownedGlobalError(GlobalError* error) {
  DCHECK(error);
  all_errors_.push_back(error);
  NotifyErrorsChanged(error);
}

std::unique_ptr<GlobalError> GlobalErrorService::RemoveGlobalError(
    GlobalError* error_ptr) {
  std::unique_ptr<GlobalError> ptr = std::move(owned_errors_[error_ptr]);
  owned_errors_.erase(error_ptr);
  RemoveUnownedGlobalError(error_ptr);
  return ptr;
}

void GlobalErrorService::RemoveUnownedGlobalError(GlobalError* error) {
  DCHECK(owned_errors_.find(error) == owned_errors_.end());
  all_errors_.erase(std::find(all_errors_.begin(), all_errors_.end(), error));
  GlobalErrorBubbleViewBase* bubble = error->GetBubbleView();
  if (bubble)
    bubble->CloseBubbleView();
  NotifyErrorsChanged(error);
}

GlobalError* GlobalErrorService::GetGlobalErrorByMenuItemCommandID(
    int command_id) const {
  for (auto* error : all_errors_)
    if (error->HasMenuItem() && command_id == error->MenuItemCommandID())
      return error;

  return nullptr;
}

GlobalError*
GlobalErrorService::GetHighestSeverityGlobalErrorWithAppMenuItem() const {
  GlobalError::Severity highest_severity = GlobalError::SEVERITY_LOW;
  GlobalError* highest_severity_error = nullptr;

  for (auto* error : all_errors_) {
    if (error->HasMenuItem()) {
      if (!highest_severity_error || error->GetSeverity() > highest_severity) {
        highest_severity = error->GetSeverity();
        highest_severity_error = error;
      }
    }
  }

  return highest_severity_error;
}

GlobalError* GlobalErrorService::GetFirstGlobalErrorWithBubbleView() const {
  for (auto* error : all_errors_) {
    if (error->HasBubbleView() && !error->HasShownBubbleView())
      return error;
  }
  return nullptr;
}

void GlobalErrorService::NotifyErrorsChanged(GlobalError* error) {
  // GlobalErrorService is bound only to original profile so we need to send
  // notifications to both it and its off-the-record profile to update
  // incognito windows as well.
  std::vector<Profile*> profiles_to_notify;
  if (profile_) {
    profiles_to_notify.push_back(profile_);
    if (profile_->IsOffTheRecord())
      profiles_to_notify.push_back(profile_->GetOriginalProfile());
    else if (profile_->HasOffTheRecordProfile())
      profiles_to_notify.push_back(profile_->GetOffTheRecordProfile());
    for (size_t i = 0; i < profiles_to_notify.size(); ++i) {
      content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_GLOBAL_ERRORS_CHANGED,
        content::Source<Profile>(profiles_to_notify[i]),
        content::Details<GlobalError>(error));
    }
  }
}
