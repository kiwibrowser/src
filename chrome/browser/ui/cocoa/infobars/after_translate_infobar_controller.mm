// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/infobars/after_translate_infobar_controller.h"

#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_utilities.h"
#include "components/translate/core/common/translate_constants.h"

using InfoBarUtilities::MoveControl;

@implementation AfterTranslateInfobarController

- (void)loadLabelText {
  autodeterminedSourceLanguage_ = ([self delegate]->original_language_code() ==
                                   translate::kUnknownLanguageCode);
  std::vector<base::string16> strings;
  translate::TranslateInfoBarDelegate::GetAfterTranslateStrings(
      &strings, &swappedLanugageButtons_, autodeterminedSourceLanguage_);
  DCHECK_EQ(autodeterminedSourceLanguage_ ? 2U : 3U, strings.size());
  [label1_ setStringValue:base::SysUTF16ToNSString(strings[0])];
  [label2_ setStringValue:base::SysUTF16ToNSString(strings[1])];
  if (strings.size() == 3U)
    [label3_ setStringValue:base::SysUTF16ToNSString(strings[2])];
}

- (void)layout {
  [self removeOkCancelButtons];
  [optionsPopUp_ setHidden:NO];
  NSView* firstPopup = fromLanguagePopUp_;
  NSView* lastPopup = toLanguagePopUp_;
  if (swappedLanugageButtons_ || autodeterminedSourceLanguage_) {
    firstPopup = toLanguagePopUp_;
    lastPopup = fromLanguagePopUp_;
  }
  NSView* lastControl = lastPopup;

  MoveControl(label1_, firstPopup, spaceBetweenControls_ / 2, true);
  if (autodeterminedSourceLanguage_) {
    MoveControl(firstPopup, label2_, 0, true);
    lastControl = label2_;
  } else {
    MoveControl(firstPopup, label2_, spaceBetweenControls_ / 2, true);
    MoveControl(label2_, lastPopup, spaceBetweenControls_ / 2, true);
    MoveControl(lastPopup, label3_, 0, true);
    lastControl = label3_;
  }

  MoveControl(lastControl, showOriginalButton_, spaceBetweenControls_ * 2,
      true);
}

- (NSArray*)visibleControls {
  if (autodeterminedSourceLanguage_) {
    return [NSArray arrayWithObjects:label1_.get(), toLanguagePopUp_.get(),
        label2_.get(), showOriginalButton_.get(), nil];
  }
  return [NSArray arrayWithObjects:label1_.get(), fromLanguagePopUp_.get(),
      label2_.get(), toLanguagePopUp_.get(), label3_.get(),
      showOriginalButton_.get(), nil];
}

- (bool)verifyLayout {
  if ([optionsPopUp_ isHidden])
    return false;
  return [super verifyLayout];
}

@end
