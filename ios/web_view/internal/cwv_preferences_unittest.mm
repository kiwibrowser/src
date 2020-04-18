// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/cwv_preferences_internal.h"

#import <Foundation/Foundation.h>
#include <memory>

#include "base/memory/scoped_refptr.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/translate/core/browser/translate_pref_names.h"
#import "ios/web_view/public/cwv_preferences_autofill.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

class CWVPreferencesTest : public PlatformTest {
 protected:
  CWVPreferencesTest() {
    scoped_refptr<user_prefs::PrefRegistrySyncable> pref_registry =
        new user_prefs::PrefRegistrySyncable;
    pref_registry->RegisterBooleanPref(autofill::prefs::kAutofillEnabled, true);
    pref_registry->RegisterBooleanPref(prefs::kOfferTranslateEnabled, true);

    scoped_refptr<PersistentPrefStore> pref_store = new InMemoryPrefStore();
    PrefServiceFactory factory;
    factory.set_user_prefs(pref_store);

    pref_service_ = factory.Create(pref_registry.get());
    preferences_ =
        [[CWVPreferences alloc] initWithPrefService:pref_service_.get()];
  }

  std::unique_ptr<PrefService> pref_service_;
  CWVPreferences* preferences_;
};

// Tests CWVPreferences |autofillEnabled|.
TEST_F(CWVPreferencesTest, AutofillEnabled) {
  EXPECT_TRUE(preferences_.autofillEnabled);
  preferences_.autofillEnabled = NO;
  EXPECT_FALSE(preferences_.autofillEnabled);
}

// Tests CWVPreferences |translationEnabled|.
TEST_F(CWVPreferencesTest, TranslationEnabled) {
  EXPECT_TRUE(preferences_.translationEnabled);
  preferences_.translationEnabled = NO;
  EXPECT_FALSE(preferences_.translationEnabled);
}

}  // namespace ios_web_view
