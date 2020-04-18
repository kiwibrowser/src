// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/infobars/before_translate_infobar_controller.h"

#include <stddef.h>

#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_utilities.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

using InfoBarUtilities::MoveControl;

namespace {

NSButton* CreateNSButtonWithResourceIDAndParameter(
    int resourceId, const base::string16& param) {
  base::string16 title = l10n_util::GetStringFUTF16(resourceId, param);
  NSButton* button = [[NSButton alloc] init];
  [button setTitle:base::SysUTF16ToNSString(title)];
  [button setBezelStyle:NSTexturedRoundedBezelStyle];
  // Round textured buttons have a different font size than the default button.
  NSFont* font = [NSFont systemFontOfSize:
      [NSFont systemFontSizeForControlSize:NSRegularControlSize]];
  [[button cell] setFont:font];
  return button;
}

} // namespace

@implementation BeforeTranslateInfobarController

- (void)dealloc {
  [neverTranslateButton_ setTarget:nil];
  [alwaysTranslateButton_ setTarget:nil];
  [super dealloc];
}

- (id)initWithInfoBar:(InfoBarCocoa*)infobar {
  if ((self = [super initWithInfoBar:infobar])) {
    [self initializeExtraControls];
  }
  return self;
}

- (void)initializeExtraControls {
  translate::TranslateInfoBarDelegate* delegate = [self delegate];
  const base::string16& language = delegate->original_language_name();
  neverTranslateButton_.reset(
      CreateNSButtonWithResourceIDAndParameter(
          IDS_TRANSLATE_INFOBAR_NEVER_TRANSLATE, language));
  [neverTranslateButton_ setTarget:self];
  [neverTranslateButton_ setAction:@selector(neverTranslate:)];

  alwaysTranslateButton_.reset(
      CreateNSButtonWithResourceIDAndParameter(
          IDS_TRANSLATE_INFOBAR_ALWAYS_TRANSLATE, language));
  [alwaysTranslateButton_ setTarget:self];
  [alwaysTranslateButton_ setAction:@selector(alwaysTranslate:)];
}

- (void)layout {
  MoveControl(label1_, fromLanguagePopUp_, spaceBetweenControls_ / 2, true);
  MoveControl(fromLanguagePopUp_, label2_, spaceBetweenControls_, true);
  MoveControl(label2_, cancelButton_, spaceBetweenControls_, true);
  MoveControl(cancelButton_, okButton_, spaceBetweenControls_, true);
  NSView* lastControl = okButton_;
  if (neverTranslateButton_.get()) {
    MoveControl(lastControl, neverTranslateButton_.get(),
                spaceBetweenControls_, true);
    lastControl = neverTranslateButton_.get();
  }
  if (alwaysTranslateButton_.get()) {
    MoveControl(lastControl, alwaysTranslateButton_.get(),
                spaceBetweenControls_, true);
  }
}

- (void)loadLabelText {
  size_t offset = 0;
  base::string16 text =
      l10n_util::GetStringFUTF16(IDS_TRANSLATE_INFOBAR_BEFORE_MESSAGE,
                                 base::string16(), &offset);
  NSString* string1 = base::SysUTF16ToNSString(text.substr(0, offset));
  NSString* string2 = base::SysUTF16ToNSString(text.substr(offset));
  [label1_ setStringValue:string1];
  [label2_ setStringValue:string2];
  [label3_ setStringValue:@""];
}

- (NSArray*)visibleControls {
  NSMutableArray* visibleControls = [NSMutableArray arrayWithObjects:
      label1_.get(), fromLanguagePopUp_.get(), label2_.get(),
      cancelButton_, okButton_, nil];

  if ([self delegate]->ShouldShowNeverTranslateShortcut())
    [visibleControls addObject:neverTranslateButton_.get()];

  if ([self delegate]->ShouldShowAlwaysTranslateShortcut())
    [visibleControls addObject:alwaysTranslateButton_.get()];

  return visibleControls;
}

// This is called when the "Never Translate [language]" button is pressed.
- (void)neverTranslate:(id)sender {
  if (![self isOwned])
    return;
  [self delegate]->NeverTranslatePageLanguage();
}

// This is called when the "Always Translate [language]" button is pressed.
- (void)alwaysTranslate:(id)sender {
  if (![self isOwned])
    return;
  [self delegate]->AlwaysTranslatePageLanguage();
}

- (bool)verifyLayout {
  if ([optionsPopUp_ isHidden])
    return false;
  return [super verifyLayout];
}

@end

@implementation BeforeTranslateInfobarController (TestingAPI)

- (NSButton*)alwaysTranslateButton {
  return alwaysTranslateButton_.get();
}
- (NSButton*)neverTranslateButton {
  return neverTranslateButton_.get();
}

@end
