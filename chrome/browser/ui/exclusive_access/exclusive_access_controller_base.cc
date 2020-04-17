// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/exclusive_access/exclusive_access_controller_base.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"

using content::WebContents;

ExclusiveAccessControllerBase::ExclusiveAccessControllerBase(
    ExclusiveAccessManager* manager)
    : manager_(manager) {}

ExclusiveAccessControllerBase::~ExclusiveAccessControllerBase() {
}

GURL ExclusiveAccessControllerBase::GetExclusiveAccessBubbleURL() const {
  return manager_->GetExclusiveAccessBubbleURL();
}

GURL ExclusiveAccessControllerBase::GetURLForExclusiveAccessBubble() const {
  return GURL();
}

void ExclusiveAccessControllerBase::OnTabDeactivated(
    WebContents* web_contents) {
}

void ExclusiveAccessControllerBase::OnTabDetachedFromView(
    WebContents* old_contents) {
  // Derived class will have to implement if necessary.
}

void ExclusiveAccessControllerBase::OnTabClosing(WebContents* web_contents) {
}

void ExclusiveAccessControllerBase::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(content::NOTIFICATION_NAV_ENTRY_COMMITTED, type);
  if (content::Details<content::LoadCommittedDetails>(details)
          ->is_navigation_to_different_page())
    ExitExclusiveAccessIfNecessary();
}

void ExclusiveAccessControllerBase::RecordBubbleReshownUMA() {
  ++bubble_reshow_count_;
}

void ExclusiveAccessControllerBase::RecordExitingUMA() {
  // Record the number of bubble reshows during this session. Only if simplified
  // fullscreen is enabled.
  RecordBubbleReshowsHistogram(bubble_reshow_count_);

  bubble_reshow_count_ = 0;
}

void ExclusiveAccessControllerBase::SetTabWithExclusiveAccess(
    WebContents* tab) {
  // Tab should never be replaced with another tab, or
  // UpdateNotificationRegistrations would need updating.
  tab_with_exclusive_access_ = tab;
  UpdateNotificationRegistrations();
}

void ExclusiveAccessControllerBase::UpdateNotificationRegistrations() {
}
