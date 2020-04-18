// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_TOUCH_TO_SEARCH_PERMISSIONS_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_TOUCH_TO_SEARCH_PERMISSIONS_MEDIATOR_H_

#import <Foundation/Foundation.h>

#import "url/gurl.h"

namespace ios {
class ChromeBrowserState;
}

namespace TouchToSearch {

// Enum describing the possible state that a user's Touch-to-Search preference
// is in:
typedef NS_ENUM(NSInteger, TouchToSearchPreferenceState) {
  UNDECIDED = -1,  // User has not set a preference.
  DISABLED,        // User has disabled TTS.
  ENABLED          // User has "opted in" and enabled TTS.
};

}  // namespace TouchToSearch

@protocol TouchToSearchPermissionsChangeAudience<NSObject>
@optional

// Called synchronously when preferences are changed
- (void)touchToSearchDidChangePreferenceState:
    (TouchToSearch::TouchToSearchPreferenceState)preferenceState;

// Called (asynchronously) after some state has changed that might have affected
// touch-to-search permissions.
- (void)touchToSearchPermissionsUpdated;

@end

// An object for managing and vending permissions associated with the
// Touch-to-Search feature.
@interface TouchToSearchPermissionsMediator : NSObject

// YES if the device supports Touch-to-Search (based on command line flags). The
// return value will be the same over the lifetime of a Chrome process.
+ (BOOL)isTouchToSearchAvailableOnDevice;

// Designated initializer.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Current preference state. Assigning to this property will update the internal
// representation backing this state.
@property(nonatomic, assign)
    TouchToSearch::TouchToSearchPreferenceState preferenceState;

// Current audience object.
@property(nonatomic, weak)
    NSObject<TouchToSearchPermissionsChangeAudience>* audience;

// YES if, given the current permissions state, touch-to-search can be enabled.
- (BOOL)canEnable;

// YES if, given the current permissions state, surrounding text in |URL| may be
// extracted.
- (BOOL)canExtractTapContextForURL:(const GURL&)url;

// YES if it is permitted to send page URLs to the contextual search service.
- (BOOL)canSendPageURLs;

// YES if search results pages can be preloaded.
- (BOOL)canPreloadSearchResults;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_TOUCH_TO_SEARCH_PERMISSIONS_MEDIATOR_H_
