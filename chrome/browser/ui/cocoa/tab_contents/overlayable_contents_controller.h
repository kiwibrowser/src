// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_OVERLAYABLE_CONTENTS_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_OVERLAYABLE_CONTENTS_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"

// OverlayableContentsController is an obsolete wrapper holding the view where a
// tab's WebContents is displayed. In the old Chrome Instant implementation it
// multiplexed between the tab's contents and an overlay's contents. Now there
// is no overlay, but ripping this class out entirely is hard.
//
// TODO(sail): Remove this class and replace it with something saner.
@interface OverlayableContentsController : NSViewController {
 @private
  // Container view for the "active" contents.
  base::scoped_nsobject<NSView> activeContainer_;
}

@property(readonly, nonatomic) NSView* activeContainer;

@end

#endif  // CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_OVERLAYABLE_CONTENTS_CONTROLLER_H_
