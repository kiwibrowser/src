// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_SERVICE_BRIDGE_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_SERVICE_BRIDGE_OBSERVER_H_

#include "components/ntp_snippets/content_suggestions_service.h"

// Observes ContentSuggestionsService events from Objective-C. To use as a
// ntp_snippets::ContentSuggestionsService::Observer, wrap in a
// ContentSuggestionsServiceBridge.
@protocol ContentSuggestionsServiceObserver

// Invoked by ntp_snippets::ContentSuggestionsService::OnNewSuggestions.
- (void)contentSuggestionsService:
            (ntp_snippets::ContentSuggestionsService*)suggestionsService
         newSuggestionsInCategory:(ntp_snippets::Category)category;

// Invoked by ntp_snippets::ContentSuggestionsService::OnCategoryStatusChanged.
- (void)contentSuggestionsService:
            (ntp_snippets::ContentSuggestionsService*)suggestionsService
                         category:(ntp_snippets::Category)category
                  statusChangedTo:(ntp_snippets::CategoryStatus)status;

// Invoked by ntp_snippets::ContentSuggestionsService::OnSuggestionInvalidated.
- (void)contentSuggestionsService:
            (ntp_snippets::ContentSuggestionsService*)suggestionsService
            suggestionInvalidated:
                (const ntp_snippets::ContentSuggestion::ID&)suggestion_id;

// Invoked by ntp_snippets::ContentSuggestionsService::OnFullRefreshRequired.
- (void)contentSuggestionsServiceFullRefreshRequired:
    (ntp_snippets::ContentSuggestionsService*)suggestionsService;

// Invoked by
// ntp_snippets::ContentSuggestionsService::ContentSuggestionsServiceShutdown.
- (void)contentSuggestionsServiceShutdown:
    (ntp_snippets::ContentSuggestionsService*)suggestionsService;

@end

// Observer for the ContentSuggestionsService that translates all the callbacks
// to Objective-C calls.
class ContentSuggestionsServiceBridge
    : public ntp_snippets::ContentSuggestionsService::Observer {
 public:
  ContentSuggestionsServiceBridge(
      id<ContentSuggestionsServiceObserver> observer,
      ntp_snippets::ContentSuggestionsService* service);
  ~ContentSuggestionsServiceBridge() override;

 private:
  void OnNewSuggestions(ntp_snippets::Category category) override;
  void OnCategoryStatusChanged(
      ntp_snippets::Category category,
      ntp_snippets::CategoryStatus new_status) override;
  void OnSuggestionInvalidated(
      const ntp_snippets::ContentSuggestion::ID& suggestion_id) override;
  void OnFullRefreshRequired() override;
  void ContentSuggestionsServiceShutdown() override;

  __weak id<ContentSuggestionsServiceObserver> observer_ = nil;
  ntp_snippets::ContentSuggestionsService* service_;  // weak

  DISALLOW_COPY_AND_ASSIGN(ContentSuggestionsServiceBridge);
};

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_SERVICE_BRIDGE_OBSERVER_H_
