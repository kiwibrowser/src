// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/translate/never_translate_infobar_controller.h"

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "components/translate/core/browser/translate_infobar_delegate.h"
#include "ios/chrome/browser/infobars/infobar_controller_delegate.h"
#include "ios/chrome/browser/translate/translate_infobar_tags.h"
#import "ios/chrome/browser/ui/infobars/confirm_infobar_view.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface NeverTranslateInfoBarController () {
  translate::TranslateInfoBarDelegate* _translateInfoBarDelegate;
}

// Action for any of the user defined buttons.
- (void)infoBarButtonDidPress:(id)sender;

@end

@implementation NeverTranslateInfoBarController

#pragma mark -
#pragma mark InfoBarControllerProtocol

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
  // Main text.
  base::string16 originalLanguage =
      _translateInfoBarDelegate->original_language_name();
  [infoBarView
      addLabel:l10n_util::GetNSStringF(IDS_IOS_TRANSLATE_INFOBAR_NEVER_MESSAGE,
                                       originalLanguage)];
  // Close button.
  [infoBarView addCloseButtonWithTag:TranslateInfoBarIOSTag::CLOSE
                              target:self
                              action:@selector(infoBarButtonDidPress:)];
  // Other buttons.
  NSString* buttonLanguage = l10n_util::GetNSStringF(
      IDS_TRANSLATE_INFOBAR_NEVER_TRANSLATE, originalLanguage);
  NSString* buttonSite = l10n_util::GetNSString(
      IDS_TRANSLATE_INFOBAR_OPTIONS_NEVER_TRANSLATE_SITE);
  [infoBarView addButton1:buttonLanguage
                     tag1:TranslateInfoBarIOSTag::DENY_LANGUAGE
                  button2:buttonSite
                     tag2:TranslateInfoBarIOSTag::DENY_WEBSITE
                   target:self
                   action:@selector(infoBarButtonDidPress:)];
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
    case TranslateInfoBarIOSTag::DENY_LANGUAGE:
      _translateInfoBarDelegate->NeverTranslatePageLanguage();
      self.delegate->RemoveInfoBar();
      break;
    case TranslateInfoBarIOSTag::DENY_WEBSITE:
      if (!_translateInfoBarDelegate->IsSiteBlacklisted())
        _translateInfoBarDelegate->ToggleSiteBlacklist();
      self.delegate->RemoveInfoBar();
      break;
    default:
      NOTREACHED() << "Unexpected Translate button label";
      break;
  }
}

@end
