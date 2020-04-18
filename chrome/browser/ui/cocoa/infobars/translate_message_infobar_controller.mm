// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/infobars/translate_message_infobar_controller.h"

#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_utilities.h"

using InfoBarUtilities::MoveControl;

@implementation TranslateMessageInfobarController

- (void)layout {
  [self removeOkCancelButtons];
  MoveControl(
      label1_, translateMessageButton_, spaceBetweenControls_ * 2, true);
  translate::TranslateInfoBarDelegate* delegate = [self delegate];
  if ([self delegate]->ShouldShowMessageInfoBarButton()) {
    base::string16 buttonText = delegate->GetMessageInfoBarButtonText();
    [translateMessageButton_ setTitle:base::SysUTF16ToNSString(buttonText)];
    [translateMessageButton_ sizeToFit];
  }
}

- (void)adjustOptionsButtonSizeAndVisibilityForView:(NSView*)lastView {
  // Do nothing, but stop the options button from showing up.
}

- (NSArray*)visibleControls {
  NSMutableArray* visibleControls =
      [NSMutableArray arrayWithObjects:label1_.get(), nil];
  if ([self delegate]->ShouldShowMessageInfoBarButton())
    [visibleControls addObject:translateMessageButton_];
  return visibleControls;
}

- (void)loadLabelText {
  translate::TranslateInfoBarDelegate* delegate = [self delegate];
  base::string16 messageText = delegate->GetMessageInfoBarText();
  NSString* string1 = base::SysUTF16ToNSString(messageText);
  [label1_ setStringValue:string1];
}

- (bool)verifyLayout {
  if (![optionsPopUp_ isHidden])
    return false;
  return [super verifyLayout];
}

- (BOOL)shouldShowOptionsPopUp {
  return NO;
}

@end
