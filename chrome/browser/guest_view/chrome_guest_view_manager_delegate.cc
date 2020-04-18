// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/guest_view/chrome_guest_view_manager_delegate.h"

#include "base/feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "content/public/browser/guest_mode.h"
#include "content/public/common/content_features.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/app_mode/app_session.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager.h"
#endif

namespace extensions {

ChromeGuestViewManagerDelegate::ChromeGuestViewManagerDelegate(
    content::BrowserContext* context)
    : ExtensionsGuestViewManagerDelegate(context) {
}

ChromeGuestViewManagerDelegate::~ChromeGuestViewManagerDelegate() {
}

void ChromeGuestViewManagerDelegate::OnGuestAdded(
    content::WebContents* guest_web_contents) const {
  ExtensionsGuestViewManagerDelegate::OnGuestAdded(guest_web_contents);

  // Attaches the task-manager-specific tag for the GuestViews to its
  // |guest_web_contents| so that their corresponding tasks show up in the task
  // manager.
  task_manager::WebContentsTags::CreateForGuestContents(guest_web_contents);

#if defined(OS_CHROMEOS)
  // Notifies kiosk session about the added guest.
  chromeos::AppSession* app_session =
      chromeos::KioskAppManager::Get()->app_session();
  if (app_session)
    app_session->OnGuestAdded(guest_web_contents);
#endif

  RegisterSyntheticFieldTrial(guest_web_contents);
}

void ChromeGuestViewManagerDelegate::RegisterSyntheticFieldTrial(
    content::WebContents* guest_web_contents) const {
  if (!guest_view::GuestViewBase::FromWebContents(guest_web_contents)
           ->CanUseCrossProcessFrames()) {
    return;
  }

  if (content::GuestMode::IsCrossProcessFrameGuest(guest_web_contents)) {
    if (base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
            ::features::kGuestViewCrossProcessFrames.name,
            base::FeatureList::OVERRIDE_ENABLE_FEATURE)) {
      ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
          "CrossProcessFramesGuestActive", "ForceEnabled");
    } else {
      ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
          "CrossProcessFramesGuestActive", "Enabled");
    }
  } else {
    if (base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
            ::features::kGuestViewCrossProcessFrames.name,
            base::FeatureList::OVERRIDE_DISABLE_FEATURE)) {
      ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
          "CrossProcessFramesGuestActive", "ForceDisabled");
    } else {
      ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
          "CrossProcessFramesGuestActive", "Disabled");
    }
  }
}

}  // namespace extensions
