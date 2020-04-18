// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/language/language_model_factory.h"

#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/platform_test.h"

using testing::Eq;
using testing::IsNull;
using testing::Not;

using LanguageModelFactoryTest = PlatformTest;

// Check that Incognito language modeling is inherited from the user's profile.
TEST_F(LanguageModelFactoryTest, SharedWithIncognito) {
  web::TestWebThreadBundle thread_bundle;

  std::unique_ptr<TestChromeBrowserState> state(
      TestChromeBrowserState::Builder().Build());
  const language::LanguageModel* const model =
      LanguageModelFactory::GetForBrowserState(state.get());
  EXPECT_THAT(model, Not(IsNull()));

  ios::ChromeBrowserState* const incognito =
      state->GetOffTheRecordChromeBrowserState();
  ASSERT_THAT(incognito, Not(IsNull()));
  EXPECT_THAT(LanguageModelFactory::GetForBrowserState(incognito), Eq(model));
}
