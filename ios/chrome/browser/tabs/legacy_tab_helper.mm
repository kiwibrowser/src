// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tabs/legacy_tab_helper.h"

#import "ios/chrome/browser/tabs/tab.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(LegacyTabHelper);

// static
void LegacyTabHelper::CreateForWebState(web::WebState* web_state) {
  CreateForWebStateInternal(web_state, nil);
}

// static
void LegacyTabHelper::CreateForWebStateForTesting(web::WebState* web_state,
                                                  Tab* tab) {
  DCHECK(tab);
  DCHECK(!FromWebState(web_state));
  CreateForWebStateInternal(web_state, tab);
}

// static
Tab* LegacyTabHelper::GetTabForWebState(web::WebState* web_state) {
  DCHECK(web_state);
  LegacyTabHelper* tab_helper = LegacyTabHelper::FromWebState(web_state);
  return tab_helper ? tab_helper->tab_ : nil;
}

LegacyTabHelper::LegacyTabHelper(web::WebState* web_state, Tab* tab)
    : tab_(tab) {}

LegacyTabHelper::~LegacyTabHelper() = default;

// static
void LegacyTabHelper::CreateForWebStateInternal(web::WebState* web_state,
                                                Tab* tab) {
  DCHECK(web_state);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(
        UserDataKey(),
        std::unique_ptr<base::SupportsUserData::Data>(new LegacyTabHelper(
            web_state, tab ? tab : [[Tab alloc] initWithWebState:web_state])));
  }
}
