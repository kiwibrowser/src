// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/voice/voice_search_navigations_tab_helper.h"

#include <memory>

#include "base/logging.h"
#include "ios/web/public/load_committed_details.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(VoiceSearchNavigationTabHelper);

namespace {
// The key under which the VoiceSearchNavigationMarkers are stored.
const void* const kNavigationMarkerKey = &kNavigationMarkerKey;
}

#pragma mark - VoiceSearchNavigationMarker

// A marker object that can be added to a NavigationItem.  Its presence
// indicates that the navigation is the result of a voice search query.
class VoiceSearchNavigationMarker : public base::SupportsUserData::Data {};

#pragma mark - VoiceSearchNavigations

VoiceSearchNavigationTabHelper::VoiceSearchNavigationTabHelper(
    web::WebState* web_state)
    : web_state_(web_state) {
  web_state_->AddObserver(this);
}

void VoiceSearchNavigationTabHelper::WillLoadVoiceSearchResult() {
  will_navigate_to_voice_search_result_ = true;
}

bool VoiceSearchNavigationTabHelper::IsExpectingVoiceSearch() const {
  return will_navigate_to_voice_search_result_;
}

bool VoiceSearchNavigationTabHelper::IsNavigationFromVoiceSearch(
    const web::NavigationItem* item) const {
  DCHECK(item);
  // Check if a voice search navigation is expected if |item| is pending load.
  const web::NavigationManager* manager = web_state_->GetNavigationManager();
  const web::NavigationItem* pending_item = manager->GetPendingItem();
  const web::NavigationItem* transient_item = manager->GetTransientItem();
  if (item && (item == pending_item || item == transient_item))
    return will_navigate_to_voice_search_result_;
  // Check if the marker exists if it's a committed navigation.
  DCHECK_NE(manager->GetIndexOfItem(item), -1);
  return item->GetUserData(kNavigationMarkerKey) != nullptr;
}

void VoiceSearchNavigationTabHelper::NavigationItemCommitted(
    web::WebState* web_state,
    const web::LoadCommittedDetails& load_details) {
  DCHECK_EQ(web_state_, web_state);
  if (will_navigate_to_voice_search_result_) {
    load_details.item->SetUserData(
        kNavigationMarkerKey, std::make_unique<VoiceSearchNavigationMarker>());
    will_navigate_to_voice_search_result_ = false;
  }
}

void VoiceSearchNavigationTabHelper::WebStateDestroyed(
    web::WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}
