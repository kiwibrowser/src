// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/translate/translate_message_infobar_controller.h"

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/translate/core/browser/translate_infobar_delegate.h"
#include "ios/chrome/browser/infobars/infobar_controller_delegate.h"
#include "ios/chrome/browser/translate/translate_infobar_tags.h"
#import "ios/chrome/browser/ui/infobars/confirm_infobar_view.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TranslateMessageInfoBarController () {
  translate::TranslateInfoBarDelegate* _translateInfoBarDelegate;
}

@end

@implementation TranslateMessageInfoBarController

- (instancetype)initWithInfoBarDelegate:
    (translate::TranslateInfoBarDelegate*)delegate {
  self = [super init];
  if (self) {
    _translateInfoBarDelegate = delegate;
  }
  return self;
}

- (UIView<InfoBarViewSizing>*)viewForFrame:(CGRect)frame {
  ConfirmInfoBarView* infoBarView =
      [[ConfirmInfoBarView alloc] initWithFrame:frame];
  // Icon
  gfx::Image icon = _translateInfoBarDelegate->GetIcon();
  if (!icon.IsEmpty())
    [infoBarView addLeftIcon:icon.ToUIImage()];
  // Text.
  [infoBarView
      addLabel:base::SysUTF16ToNSString(
                   _translateInfoBarDelegate->GetMessageInfoBarText())];
  // Close button.
  [infoBarView addCloseButtonWithTag:TranslateInfoBarIOSTag::CLOSE
                              target:self
                              action:@selector(infoBarButtonDidPress:)];
  // Other button.
  base::string16 buttonText(
      _translateInfoBarDelegate->GetMessageInfoBarButtonText());
  if (!buttonText.empty()) {
    [infoBarView addButton:base::SysUTF16ToNSString(buttonText)
                       tag:TranslateInfoBarIOSTag::MESSAGE
                    target:self
                    action:@selector(infoBarButtonDidPress:)];
  }
  return infoBarView;
}

#pragma mark - Handling of User Events

- (void)infoBarButtonDidPress:(id)sender {
  // This press might have occurred after the user has already pressed a button,
  // in which case the view has been detached from the delegate and this press
  // should be ignored.
  if (!self.delegate) {
    return;
  }

  NSUInteger buttonId = base::mac::ObjCCastStrict<UIButton>(sender).tag;
  switch (buttonId) {
    case TranslateInfoBarIOSTag::CLOSE:
      _translateInfoBarDelegate->InfoBarDismissed();
      self.delegate->RemoveInfoBar();
      break;
    case TranslateInfoBarIOSTag::MESSAGE:
      _translateInfoBarDelegate->MessageInfoBarButtonPressed();
      break;
    default:
      NOTREACHED() << "Unexpected Translate button label";
      break;
  }
}

@end
