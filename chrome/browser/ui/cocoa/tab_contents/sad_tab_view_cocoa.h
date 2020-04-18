// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_SAD_TAB_VIEW_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_SAD_TAB_VIEW_COCOA_H_

#import <Cocoa/Cocoa.h>

#include "chrome/browser/ui/sad_tab.h"

// A view that displays the "sad tab" (aka crash page).
@interface SadTabView : NSView

- (instancetype)initWithFrame:(NSRect)frame sadTab:(SadTab*)sadTab;

@end

#endif  // CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_SAD_TAB_VIEW_COCOA_H_
