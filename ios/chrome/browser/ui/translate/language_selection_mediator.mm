// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/translate/language_selection_mediator.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/translate/core/browser/translate_infobar_delegate.h"
#import "ios/chrome/browser/translate/language_selection_context.h"
#import "ios/chrome/browser/ui/translate/language_selection_consumer.h"
#import "ios/chrome/browser/ui/translate/language_selection_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface LanguageSelectionMediator ()<LanguageSelectionProvider>
@property(nonatomic) LanguageSelectionContext* context;
@end

@implementation LanguageSelectionMediator

@synthesize consumer = _consumer;
@synthesize context = _context;

- (instancetype)initWithContext:(LanguageSelectionContext*)context {
  if ((self = [super init])) {
    _context = context;
  }
  return self;
}

- (void)setConsumer:(id<LanguageSelectionConsumer>)consumer {
  _consumer = consumer;
  self.consumer.languageCount = self.context.languageData->num_languages();
  self.consumer.initialLanguageIndex = self.context.initialLanguageIndex;
  self.consumer.disabledLanguageIndex = self.context.unavailableLanguageIndex;
  self.consumer.provider = self;
}

#pragma mark - Public methods

- (std::string)languageCodeForLanguageAtIndex:(int)index {
  return self.context.languageData->language_code_at(index);
}

#pragma mark - LanguageSelectionProvider

- (NSString*)languageNameAtIndex:(int)languageIndex {
  if (languageIndex < 0 ||
      languageIndex >=
          static_cast<int>(self.context.languageData->num_languages())) {
    NOTREACHED() << "Language index " << languageIndex
                 << " out of expected range.";
    return nil;
  }
  return base::SysUTF16ToNSString(
      self.context.languageData->language_name_at(languageIndex));
}

@end
