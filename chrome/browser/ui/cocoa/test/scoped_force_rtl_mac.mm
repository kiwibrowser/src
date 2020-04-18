// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/test/scoped_force_rtl_mac.h"

#import <Foundation/Foundation.h>

#include "base/i18n/rtl.h"
#include "chrome/common/chrome_features.h"

namespace {

NSString* const kAppleTextDirectionDefaultsKey = @"AppleTextDirection";
NSString* const kForceRTLWritingDirectionDefaultsKey =
    @"NSForceRightToLeftWritingDirection";
const char kDefaultRTLLocale[] = "he";  // Hebrew.

}  // namespace

namespace cocoa_l10n_util {

ScopedForceRTLMac::ScopedForceRTLMac() {
  scoped_feature_list_.InitAndEnableFeature(features::kMacRTL);
  NSUserDefaults* standard_defaults = [NSUserDefaults standardUserDefaults];
  original_apple_text_direction_ =
      [standard_defaults boolForKey:kAppleTextDirectionDefaultsKey];
  original_rtl_writing_direction_ =
      [standard_defaults boolForKey:kForceRTLWritingDirectionDefaultsKey];
  [standard_defaults setBool:YES forKey:kAppleTextDirectionDefaultsKey];
  [standard_defaults setBool:YES forKey:kForceRTLWritingDirectionDefaultsKey];
  original_locale_ = base::i18n::GetConfiguredLocale();
  base::i18n::SetICUDefaultLocale(kDefaultRTLLocale);
}

ScopedForceRTLMac::~ScopedForceRTLMac() {
  [[NSUserDefaults standardUserDefaults]
      setBool:original_apple_text_direction_
       forKey:kAppleTextDirectionDefaultsKey];
  [[NSUserDefaults standardUserDefaults]
      setBool:original_rtl_writing_direction_
       forKey:kForceRTLWritingDirectionDefaultsKey];
  base::i18n::SetICUDefaultLocale(original_locale_);
}

}  // namespace cocoa_l10n_util
