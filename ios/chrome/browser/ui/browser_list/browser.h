// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_H_

#include <memory>

#include "base/macros.h"
#include "base/supports_user_data.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"

class WebStateList;
class WebStateListDelegate;

@class ChromeBroadcaster;

namespace ios {
class ChromeBrowserState;
}

// Browser is the model for a window containing multiple tabs. Instances
// are owned by a BrowserList to allow multiple windows for a single user
// session.
//
// See src/docs/ios/objects.md for more information.
class Browser : public base::SupportsUserData {
 public:
  // Constructs a Browser attached to |browser_state|. The |delegate| is
  // passed to WebStateList constructor. It must be non-null and outlive
  // the Browser.
  Browser(ios::ChromeBrowserState* browser_state,
          WebStateListDelegate* delegate);
  ~Browser() override;

  // Accessor for the ChromeBroadcaster.
  ChromeBroadcaster* broadcaster() { return broadcaster_; }

  // Accessor for the owning ChromeBrowserState.
  ios::ChromeBrowserState* browser_state() const { return browser_state_; }

  // Accessors for the WebStateList.
  WebStateList& web_state_list() { return web_state_list_; }
  const WebStateList& web_state_list() const { return web_state_list_; }

 private:
  __strong ChromeBroadcaster* broadcaster_;
  ios::ChromeBrowserState* browser_state_;
  WebStateList web_state_list_;

  DISALLOW_COPY_AND_ASSIGN(Browser);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_H_
