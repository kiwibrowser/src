// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/language/url_language_histogram_factory.h"

#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::IsNull;
using testing::Not;

TEST(UrlLanguageHistogramFactoryTest, NotCreatedInIncognito) {
  content::TestBrowserThreadBundle thread_bundle;
  TestingProfile profile;

  EXPECT_THAT(UrlLanguageHistogramFactory::GetForBrowserContext(&profile),
              Not(IsNull()));

  Profile* incognito = profile.GetOffTheRecordProfile();
  ASSERT_THAT(incognito, Not(IsNull()));
  EXPECT_THAT(UrlLanguageHistogramFactory::GetForBrowserContext(incognito),
              IsNull());
}
