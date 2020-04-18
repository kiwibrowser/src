// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DRAGGABLE_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_DRAGGABLE_BUTTON_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/draggable_button_mixin.h"

// Class for buttons that can be drag sources. If the mouse is clicked and moved
// more than a given distance, this class will call |-beginDrag:| instead of
// |-performClick:|. Subclasses should override these two methods.
@interface DraggableButton : NSButton<DraggableButtonMixin> {
 @private
  base::scoped_nsobject<DraggableButtonImpl> draggableButtonImpl_;
}

@property(readonly, nonatomic) DraggableButtonImpl* draggableButton;

@end

#endif  // CHROME_BROWSER_UI_COCOA_DRAGGABLE_BUTTON_H_
