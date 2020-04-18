// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ntp_snippets/content_suggestions_service_factory.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

using ContentSuggestionsServiceFactoryTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(ContentSuggestionsServiceFactoryTest,
                       CreatesServiceOnlyOnAndroid) {
  ntp_snippets::ContentSuggestionsService* service =
      ContentSuggestionsServiceFactory::GetForProfile(browser()->profile());
#if defined(OS_ANDROID)
  EXPECT_TRUE(service);
#else
  EXPECT_FALSE(service);
#endif
}
