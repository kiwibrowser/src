// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"

#include "base/time/time.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"

namespace {

const int kDefaultNewTabPromoShowTimeInHours = 2;
constexpr char kNewTabObservedSessionTimeKey[] =
    "new_tab_in_product_help_observed_session_time_key";

}  // namespace

namespace feature_engagement {

NewTabTracker::NewTabTracker(Profile* profile)
    : FeatureTracker(
          profile,
          &kIPHNewTabFeature,
          kNewTabObservedSessionTimeKey,
          base::TimeDelta::FromHours(kDefaultNewTabPromoShowTimeInHours)) {}

NewTabTracker::~NewTabTracker() = default;

void NewTabTracker::OnNewTabOpened() {
  GetTracker()->NotifyEvent(events::kNewTabOpened);
}

void NewTabTracker::OnOmniboxNavigation() {
  GetTracker()->NotifyEvent(events::kOmniboxInteraction);
}

void NewTabTracker::OnOmniboxFocused() {
  if (ShouldShowPromo())
    ShowPromo();
}

void NewTabTracker::OnPromoClosed() {
  GetTracker()->Dismissed(kIPHNewTabFeature);
}

void NewTabTracker::OnSessionTimeMet() {
  GetTracker()->NotifyEvent(events::kNewTabSessionTimeMet);
}

void NewTabTracker::ShowPromo() {
  NewTabButton::ShowPromoForLastActiveBrowser();
}

void NewTabTracker::CloseBubble() {
  NewTabButton::CloseBubbleForLastActiveBrowser();
}
}  // namespace feature_engagement
