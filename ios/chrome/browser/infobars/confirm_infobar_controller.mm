// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/infobars/confirm_infobar_controller.h"

#include "base/mac/foundation_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#import "ios/chrome/browser/infobars/infobar_controller+protected.h"
#include "ios/chrome/browser/infobars/infobar_controller_delegate.h"
#import "ios/chrome/browser/ui/infobars/confirm_infobar_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// UI Tags for the infobar elements.
typedef NS_ENUM(NSInteger, ConfirmInfoBarUITags) {
  OK = 1,
  CANCEL,
  CLOSE,
  TITLE_LINK
};

}  // namespace

#pragma mark - ConfirmInfoBarController

@interface ConfirmInfoBarController () {
  ConfirmInfoBarDelegate* _confirmInfobarDelegate;
  __weak ConfirmInfoBarView* _infoBarView;
}

@end

@implementation ConfirmInfoBarController

#pragma mark -
#pragma mark InfoBarController

- (instancetype)initWithInfoBarDelegate:(ConfirmInfoBarDelegate*)delegate {
  self = [super init];
  if (self) {
    _confirmInfobarDelegate = delegate;
  }
  return self;
}

- (UIView<InfoBarViewSizing>*)viewForFrame:(CGRect)frame {
  ConfirmInfoBarView* infoBarView =
      [[ConfirmInfoBarView alloc] initWithFrame:frame];
  _infoBarView = infoBarView;
  // Model data.
  gfx::Image modelIcon = _confirmInfobarDelegate->GetIcon();
  int buttons = _confirmInfobarDelegate->GetButtons();
  NSString* buttonOK = nil;
  if (buttons & ConfirmInfoBarDelegate::BUTTON_OK) {
    buttonOK = base::SysUTF16ToNSString(_confirmInfobarDelegate->GetButtonLabel(
        ConfirmInfoBarDelegate::BUTTON_OK));
  }
  NSString* buttonCancel = nil;
  if (buttons & ConfirmInfoBarDelegate::BUTTON_CANCEL) {
    buttonCancel =
        base::SysUTF16ToNSString(_confirmInfobarDelegate->GetButtonLabel(
            ConfirmInfoBarDelegate::BUTTON_CANCEL));
  }

  [infoBarView addCloseButtonWithTag:ConfirmInfoBarUITags::CLOSE
                              target:self
                              action:@selector(infoBarButtonDidPress:)];

  // Optional left icon.
  if (!modelIcon.IsEmpty())
    [infoBarView addLeftIcon:modelIcon.ToUIImage()];

  // Optional message.
  [self updateInfobarLabel:infoBarView];

  if (buttonOK && buttonCancel) {
    [infoBarView addButton1:buttonOK
                       tag1:ConfirmInfoBarUITags::OK
                    button2:buttonCancel
                       tag2:ConfirmInfoBarUITags::CANCEL
                     target:self
                     action:@selector(infoBarButtonDidPress:)];
  } else if (buttonOK) {
    [infoBarView addButton:buttonOK
                       tag:ConfirmInfoBarUITags::OK
                    target:self
                    action:@selector(infoBarButtonDidPress:)];
  } else {
    // No buttons, only message.
    DCHECK(!_confirmInfobarDelegate->GetMessageText().empty() && !buttonCancel);
  }
  return infoBarView;
}

- (void)updateInfobarLabel:(ConfirmInfoBarView*)view {
  if (!_confirmInfobarDelegate->GetMessageText().length())
    return;
  if (_confirmInfobarDelegate->GetLinkText().length()) {
    base::string16 msgLink = base::SysNSStringToUTF16([[view class]
        stringAsLink:base::SysUTF16ToNSString(
                         _confirmInfobarDelegate->GetLinkText())
                 tag:ConfirmInfoBarUITags::TITLE_LINK]);
    base::string16 messageText = _confirmInfobarDelegate->GetMessageText();
    base::ReplaceFirstSubstringAfterOffset(
        &messageText, 0, _confirmInfobarDelegate->GetLinkText(), msgLink);

    __weak ConfirmInfoBarController* weakSelf = self;
    [view addLabel:base::SysUTF16ToNSString(messageText)
            action:^(NSUInteger tag) {
              [weakSelf infobarLinkDidPress:tag];
            }];
  } else {
    NSString* label =
        base::SysUTF16ToNSString(_confirmInfobarDelegate->GetMessageText());
    [view addLabel:label];
  }
}

- (ConfirmInfoBarView*)view {
  return _infoBarView;
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
    case ConfirmInfoBarUITags::OK:
      if (_confirmInfobarDelegate->Accept()) {
        self.delegate->RemoveInfoBar();
      }
      break;
    case ConfirmInfoBarUITags::CANCEL:
      if (_confirmInfobarDelegate->Cancel()) {
        self.delegate->RemoveInfoBar();
      }
      break;
    case ConfirmInfoBarUITags::CLOSE:
      _confirmInfobarDelegate->InfoBarDismissed();
      self.delegate->RemoveInfoBar();
      break;
    default:
      NOTREACHED() << "Unexpected button pressed";
      break;
  }
}

// Title link was clicked.
- (void)infobarLinkDidPress:(NSUInteger)tag {
  if (!self.delegate) {
    return;
  }

  DCHECK(tag == ConfirmInfoBarUITags::TITLE_LINK);
  _confirmInfobarDelegate->LinkClicked(
      WindowOpenDisposition::NEW_FOREGROUND_TAB);
}

@end
