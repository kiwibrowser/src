// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_URL_LOADER_H_
#define IOS_CHROME_BROWSER_UI_URL_LOADER_H_

#import <UIKit/UIKit.h>

#include "base/strings/string16.h"
#import "ios/web/public/navigation_manager.h"

class GURL;

namespace sessions {
struct SessionTab;
}

namespace web {
struct Referrer;
}

// Describes the intended position for a new tab.
enum OpenPosition {
  kCurrentTab,  // Relative to currently selected tab.
  kLastTab      // Always at end of tab model.
};

@protocol UrlLoader<NSObject>

// Load a new request.
- (void)loadURLWithParams:(const web::NavigationManager::WebLoadParams&)params;

// Load a new URL on a new page/tab. The |referrer| is optional. The tab will be
// placed in the model according to |appendTo|.
- (void)webPageOrderedOpen:(const GURL&)url
                  referrer:(const web::Referrer&)referrer
              inBackground:(BOOL)inBackground
                  appendTo:(OpenPosition)appendTo;

// Load a new URL on a new page/tab. The |referrer| is optional. The tab will be
// placed in the model according to |appendTo|.
- (void)webPageOrderedOpen:(const GURL&)url
                  referrer:(const web::Referrer&)referrer
               inIncognito:(BOOL)inIncognito
              inBackground:(BOOL)inBackground
                  appendTo:(OpenPosition)appendTo;

// Load a tab with the given session.
- (void)loadSessionTab:(const sessions::SessionTab*)sessionTab;

// Loads the text entered in the location bar as javascript.
- (void)loadJavaScriptFromLocationBar:(NSString*)script;

@end

#endif  // IOS_CHROME_BROWSER_UI_URL_LOADER_H_
