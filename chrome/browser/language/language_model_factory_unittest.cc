// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/language/language_model_factory.h"

#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::Eq;
using testing::IsNull;
using testing::Not;

// Check that Incognito language modeling is inherited from the user's profile.
TEST(LanguageModelFactoryTest, SharedWithIncognito) {
  content::TestBrowserThreadBundle thread_bundle;

  TestingProfile profile;
  const language::LanguageModel* const model =
      LanguageModelFactory::GetForBrowserContext(&profile);
  EXPECT_THAT(model, Not(IsNull()));

  Profile* const incognito = profile.GetOffTheRecordProfile();
  ASSERT_THAT(incognito, Not(IsNull()));
  EXPECT_THAT(LanguageModelFactory::GetForBrowserContext(incognito), Eq(model));
}
