// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/cwv_preferences_internal.h"

#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/translate/core/browser/translate_pref_names.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "ios/web_view/cwv_web_view_features.h"
#include "ios/web_view/internal/pref_names.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation CWVPreferences {
  PrefService* _prefService;
}

- (instancetype)initWithPrefService:(PrefService*)prefService {
  self = [super init];
  if (self) {
    _prefService = prefService;
  }
  return self;
}

#pragma mark - Public Methods

- (void)setTranslationEnabled:(BOOL)enabled {
  _prefService->SetBoolean(prefs::kOfferTranslateEnabled, enabled);
}

- (BOOL)isTranslationEnabled {
  return _prefService->GetBoolean(prefs::kOfferTranslateEnabled);
}

- (void)resetTranslationSettings {
  translate::TranslatePrefs translatePrefs(
      _prefService, prefs::kAcceptLanguages,
      /*preferred_languages_pref=*/nullptr);
  translatePrefs.ResetToDefaults();
}

#if BUILDFLAG(IOS_WEB_VIEW_ENABLE_AUTOFILL)
#pragma mark - Autofill

- (void)setAutofillEnabled:(BOOL)enabled {
  _prefService->SetBoolean(autofill::prefs::kAutofillEnabled, enabled);
}

- (BOOL)isAutofillEnabled {
  return _prefService->GetBoolean(autofill::prefs::kAutofillEnabled);
}
#endif  // BUILDFLAG(IOS_WEB_VIEW_ENABLE_AUTOFILL)

@end
