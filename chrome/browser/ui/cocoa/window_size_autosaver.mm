// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/window_size_autosaver.h"

#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

// If the window width stored in the prefs is smaller than this, the size is
// not restored but instead cleared from the profile -- to protect users from
// accidentally making their windows very small and then not finding them again.
const int kMinWindowWidth = 101;

// Minimum restored window height, see |kMinWindowWidth|.
const int kMinWindowHeight = 17;

@interface WindowSizeAutosaver (Private)
- (void)save:(NSNotification*)notification;
- (void)restore;
@end

@implementation WindowSizeAutosaver

- (id)initWithWindow:(NSWindow*)window
         prefService:(PrefService*)prefs
                path:(const char*)path {
  if ((self = [super init])) {
    window_ = window;
    prefService_ = prefs;
    path_ = path;

    [self restore];
    [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(save:)
             name:NSWindowDidMoveNotification
           object:window_];
    [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(save:)
             name:NSWindowDidResizeNotification
           object:window_];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)save:(NSNotification*)notification {
  DictionaryPrefUpdate update(prefService_, path_);
  base::DictionaryValue* windowPrefs = update.Get();
  NSRect frame = [window_ frame];
  if ([window_ styleMask] & NSResizableWindowMask) {
    // Save the origin of the window.
    windowPrefs->SetInteger("left", NSMinX(frame));
    windowPrefs->SetInteger("right", NSMaxX(frame));
    // windows's and linux's profiles have top < bottom due to having their
    // screen origin in the upper left, while cocoa's is in the lower left. To
    // keep the top < bottom invariant, store top in bottom and vice versa.
    windowPrefs->SetInteger("top", NSMinY(frame));
    windowPrefs->SetInteger("bottom", NSMaxY(frame));
  } else {
    // Save the origin of the window.
    windowPrefs->SetInteger("x", frame.origin.x);
    windowPrefs->SetInteger("y", frame.origin.y);
  }
}

- (void)restore {
  // Get the positioning information.
  const base::DictionaryValue* windowPrefs = prefService_->GetDictionary(path_);
  if ([window_ styleMask] & NSResizableWindowMask) {
    int x1, x2, y1, y2;
    if (!windowPrefs->GetInteger("left", &x1) ||
        !windowPrefs->GetInteger("right", &x2) ||
        !windowPrefs->GetInteger("top", &y1) ||
        !windowPrefs->GetInteger("bottom", &y2)) {
      return;
    }
    if (x2 - x1 < kMinWindowWidth || y2 - y1 < kMinWindowHeight) {
      // Windows should never be very small.
      DictionaryPrefUpdate update(prefService_, path_);
      base::DictionaryValue* mutableWindowPrefs = update.Get();
      mutableWindowPrefs->Remove("left", NULL);
      mutableWindowPrefs->Remove("right", NULL);
      mutableWindowPrefs->Remove("top", NULL);
      mutableWindowPrefs->Remove("bottom", NULL);
    } else {
      [window_ setFrame:NSMakeRect(x1, y1, x2 - x1, y2 - y1) display:YES];

      // Make sure the window is on-screen.
      [window_ cascadeTopLeftFromPoint:NSZeroPoint];
    }
  } else {
    int x, y;
    if (!windowPrefs->GetInteger("x", &x) ||
        !windowPrefs->GetInteger("y", &y))
       return;  // Nothing stored.
    // Turn the origin (lower-left) into an upper-left window point.
    NSPoint upperLeft = NSMakePoint(x, y + NSHeight([window_ frame]));
    [window_ cascadeTopLeftFromPoint:upperLeft];
  }
}

@end

