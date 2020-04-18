// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/find_in_page/find_tab_helper.h"

#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/find_in_page/find_in_page_controller.h"
#import "ios/chrome/browser/find_in_page/find_in_page_model.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(FindTabHelper);

FindTabHelper::FindTabHelper(web::WebState* web_state) {
  web_state->AddObserver(this);
  controller_ = [[FindInPageController alloc] initWithWebState:web_state];
}

FindTabHelper::~FindTabHelper() {}

void FindTabHelper::StartFinding(NSString* search_term,
                                 FindInPageCompletionBlock completion) {
  [controller_ findStringInPage:search_term
              completionHandler:^{
                FindInPageModel* model = controller_.findInPageModel;
                completion(model);
              }];
}

void FindTabHelper::ContinueFinding(FindDirection direction,
                                    FindInPageCompletionBlock completion) {
  FindInPageModel* model = controller_.findInPageModel;

  if (direction == FORWARD) {
    [controller_ findNextStringInPageWithCompletionHandler:^{
      completion(model);
    }];

  } else if (direction == REVERSE) {
    [controller_ findPreviousStringInPageWithCompletionHandler:^{
      completion(model);
    }];

  } else {
    NOTREACHED();
  }
}

void FindTabHelper::StopFinding(ProceduralBlock completion) {
  SetFindUIActive(false);
  [controller_ disableFindInPageWithCompletionHandler:completion];
}

FindInPageModel* FindTabHelper::GetFindResult() const {
  return controller_.findInPageModel;
}

bool FindTabHelper::CurrentPageSupportsFindInPage() const {
  return [controller_ canFindInPage];
}

bool FindTabHelper::IsFindUIActive() const {
  return controller_.findInPageModel.enabled;
}

void FindTabHelper::SetFindUIActive(bool active) {
  controller_.findInPageModel.enabled = active;
}

void FindTabHelper::PersistSearchTerm() {
  [controller_ saveSearchTerm];
}

void FindTabHelper::RestoreSearchTerm() {
  [controller_ restoreSearchTerm];
}

void FindTabHelper::NavigationItemCommitted(
    web::WebState* web_state,
    const web::LoadCommittedDetails& load_details) {
  StopFinding(nil);
}

void FindTabHelper::WebStateDestroyed(web::WebState* web_state) {
  [controller_ detachFromWebState];
  web_state->RemoveObserver(this);
}
