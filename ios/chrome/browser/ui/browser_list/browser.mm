// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_list/browser.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"

#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

Browser::Browser(ios::ChromeBrowserState* browser_state,
                 WebStateListDelegate* delegate)
    : broadcaster_([[ChromeBroadcaster alloc] init]),
      browser_state_(browser_state),
      web_state_list_(delegate) {
  DCHECK(browser_state_);
}

Browser::~Browser() = default;
