// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/translate/before_translate_infobar_controller.h"

#include <stddef.h>
#import <UIKit/UIKit.h>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "components/translate/core/browser/translate_infobar_delegate.h"
#include "ios/chrome/browser/infobars/infobar_controller_delegate.h"
#include "ios/chrome/browser/translate/language_selection_context.h"
#include "ios/chrome/browser/translate/language_selection_delegate.h"
#include "ios/chrome/browser/translate/language_selection_handler.h"
#include "ios/chrome/browser/translate/translate_infobar_tags.h"
#import "ios/chrome/browser/ui/infobars/confirm_infobar_view.h"
#import "ios/chrome/browser/ui/util/top_view_controller.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BeforeTranslateInfoBarController ()<LanguageSelectionDelegate>

@end

@implementation BeforeTranslateInfoBarController {
  translate::TranslateInfoBarDelegate* _translateInfoBarDelegate;  // weak
  // Stores whether the user is currently choosing in the UIPickerView the
  // original language, or the target language.
  TranslateInfoBarIOSTag::Tag _languageSelectionType;
  __weak ConfirmInfoBarView* _infoBarView;
}

@synthesize languageSelectionHandler = _languageSelectionHandler;

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
  _infoBarView = infoBarView;
  // Icon
  gfx::Image icon = _translateInfoBarDelegate->GetIcon();
  if (!icon.IsEmpty())
    [infoBarView addLeftIcon:icon.ToUIImage()];

  // Main text.
  [self updateInfobarLabelOnView:infoBarView];

  // Close button.
  [infoBarView addCloseButtonWithTag:TranslateInfoBarIOSTag::BEFORE_DENY
                              target:self
                              action:@selector(infoBarButtonDidPress:)];
  // Other buttons.
  NSString* buttonAccept = l10n_util::GetNSString(IDS_TRANSLATE_INFOBAR_ACCEPT);
  NSString* buttonDeny = l10n_util::GetNSString(IDS_TRANSLATE_INFOBAR_DENY);
  [infoBarView addButton1:buttonAccept
                     tag1:TranslateInfoBarIOSTag::BEFORE_ACCEPT
                  button2:buttonDeny
                     tag2:TranslateInfoBarIOSTag::BEFORE_DENY
                   target:self
                   action:@selector(infoBarButtonDidPress:)];
  return infoBarView;
}

- (void)updateInfobarLabelOnView:(ConfirmInfoBarView*)view {
  NSString* originalLanguage = base::SysUTF16ToNSString(
      _translateInfoBarDelegate->original_language_name());
  NSString* targetLanguage = base::SysUTF16ToNSString(
      _translateInfoBarDelegate->target_language_name());
  base::string16 originalLanguageWithLink =
      base::SysNSStringToUTF16([[view class]
          stringAsLink:originalLanguage
                   tag:TranslateInfoBarIOSTag::BEFORE_SOURCE_LANGUAGE]);
  base::string16 targetLanguageWithLink = base::SysNSStringToUTF16([[view class]
      stringAsLink:targetLanguage
               tag:TranslateInfoBarIOSTag::BEFORE_TARGET_LANGUAGE]);
  NSString* label =
      l10n_util::GetNSStringF(IDS_TRANSLATE_INFOBAR_BEFORE_MESSAGE_IOS,
                              originalLanguageWithLink, targetLanguageWithLink);

  __weak BeforeTranslateInfoBarController* weakSelf = self;
  [view addLabel:label
          action:^(NSUInteger tag) {
            [weakSelf infobarLinkDidPress:tag];
          }];
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
    case TranslateInfoBarIOSTag::BEFORE_ACCEPT:
      _translateInfoBarDelegate->Translate();
      break;
    case TranslateInfoBarIOSTag::BEFORE_DENY:
      _translateInfoBarDelegate->TranslationDeclined();
      if (_translateInfoBarDelegate->ShouldShowNeverTranslateShortcut())
        _translateInfoBarDelegate->ShowNeverTranslateInfobar();
      else
        self.delegate->RemoveInfoBar();
      break;
    default:
      NOTREACHED() << "Unexpected Translate button label";
      break;
  }
}

- (void)infobarLinkDidPress:(NSUInteger)tag {
  _languageSelectionType = static_cast<TranslateInfoBarIOSTag::Tag>(tag);
  DCHECK(_languageSelectionType ==
             TranslateInfoBarIOSTag::BEFORE_SOURCE_LANGUAGE ||
         _languageSelectionType ==
             TranslateInfoBarIOSTag::BEFORE_TARGET_LANGUAGE);

  size_t selectedRow;
  size_t disabledRow;
  int originalLanguageIndex = -1;
  int targetLanguageIndex = -1;

  for (size_t i = 0; i < _translateInfoBarDelegate->num_languages(); ++i) {
    if (_translateInfoBarDelegate->language_code_at(i) ==
        _translateInfoBarDelegate->original_language_code()) {
      originalLanguageIndex = i;
    }
    if (_translateInfoBarDelegate->language_code_at(i) ==
        _translateInfoBarDelegate->target_language_code()) {
      targetLanguageIndex = i;
    }
  }
  DCHECK_GT(originalLanguageIndex, -1);
  DCHECK_GT(targetLanguageIndex, -1);

  if (_languageSelectionType ==
      TranslateInfoBarIOSTag::BEFORE_SOURCE_LANGUAGE) {
    selectedRow = originalLanguageIndex;
    disabledRow = targetLanguageIndex;
  } else {
    selectedRow = targetLanguageIndex;
    disabledRow = originalLanguageIndex;
  }
  LanguageSelectionContext* context = [LanguageSelectionContext
      contextWithLanguageData:_translateInfoBarDelegate
                 initialIndex:selectedRow
             unavailableIndex:disabledRow];
  [self.languageSelectionHandler showLanguageSelectorWithContext:context
                                                        delegate:self];
}

#pragma mark - LanguageSelectionDelegate

- (void)languageSelectorSelectedLanguage:(std::string)languageCode {
  if (_languageSelectionType ==
          TranslateInfoBarIOSTag::BEFORE_SOURCE_LANGUAGE &&
      languageCode != _translateInfoBarDelegate->target_language_code()) {
    _translateInfoBarDelegate->UpdateOriginalLanguage(languageCode);
  }
  if (_languageSelectionType ==
          TranslateInfoBarIOSTag::BEFORE_TARGET_LANGUAGE &&
      languageCode != _translateInfoBarDelegate->original_language_code()) {
    _translateInfoBarDelegate->UpdateTargetLanguage(languageCode);
  }
  [self updateInfobarLabelOnView:_infoBarView];
}

- (void)languageSelectorClosedWithoutSelection {
  // No-op in this implementation, but (for example) metrics for this state
  // might be added.
}

@end
