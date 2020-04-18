// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_service_bridge_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

ContentSuggestionsServiceBridge::ContentSuggestionsServiceBridge(
    id<ContentSuggestionsServiceObserver> observer,
    ntp_snippets::ContentSuggestionsService* service) {
  observer_ = observer;
  service_ = service;
  service->AddObserver(this);
}

ContentSuggestionsServiceBridge::~ContentSuggestionsServiceBridge() {
  service_->RemoveObserver(this);
}

void ContentSuggestionsServiceBridge::OnNewSuggestions(
    ntp_snippets::Category category) {
  [observer_ contentSuggestionsService:service_
              newSuggestionsInCategory:category];
}

void ContentSuggestionsServiceBridge::OnCategoryStatusChanged(
    ntp_snippets::Category category,
    ntp_snippets::CategoryStatus new_status) {
  [observer_ contentSuggestionsService:service_
                              category:category
                       statusChangedTo:new_status];
}

void ContentSuggestionsServiceBridge::OnSuggestionInvalidated(
    const ntp_snippets::ContentSuggestion::ID& suggestion_id) {
  [observer_ contentSuggestionsService:service_
                 suggestionInvalidated:suggestion_id];
}

void ContentSuggestionsServiceBridge::OnFullRefreshRequired() {
  [observer_ contentSuggestionsServiceFullRefreshRequired:service_];
}

void ContentSuggestionsServiceBridge::ContentSuggestionsServiceShutdown() {
  [observer_ contentSuggestionsServiceShutdown:service_];
}
