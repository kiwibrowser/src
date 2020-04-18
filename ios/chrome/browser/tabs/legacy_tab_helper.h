// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_LEGACY_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_TABS_LEGACY_TAB_HELPER_H_

#include "base/macros.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@class Tab;

// LegacyTabHelper associates a Tab instance to a WebState object.
class LegacyTabHelper : public web::WebStateUserData<LegacyTabHelper> {
 public:
  // Creates the LegacyTabHelper. This immediately creates the Tab object.
  static void CreateForWebState(web::WebState* web_state);

  // Creates the LegacyTabHelper with a pre-constructed Tab object for
  // testing. The Tab is owned by LegacyTabHelper and must not be nil.
  static void CreateForWebStateForTesting(web::WebState* web_state, Tab* tab);

  // Returns the Tab associated with |web_state| or nil.
  static Tab* GetTabForWebState(web::WebState* web_state);

 private:
  LegacyTabHelper(web::WebState* web_state, Tab* tab);
  ~LegacyTabHelper() override;

  // Creates the LegacyTabHelper with an associated Tab object. Creates the
  // Tab if |tab| is nil.
  static void CreateForWebStateInternal(web::WebState* web_state, Tab* tab);

  // The Tab instance associated with the WebState.
  Tab* tab_;

  DISALLOW_COPY_AND_ASSIGN(LegacyTabHelper);
};

#endif  // IOS_CHROME_BROWSER_TABS_LEGACY_TAB_HELPER_H_
