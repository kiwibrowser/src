// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_TEST_CHROME_PROVIDER_INITIALIZER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_TEST_CHROME_PROVIDER_INITIALIZER_H_

#include <memory>

#include "base/macros.h"

namespace ios {

class TestChromeBrowserProvider;

// Initializes various objects needed to run unit tests that use ios::
// objects. Currently this includes setting up the chrome browser provider as
// a |TestChromeBrowserProvider|.
class TestChromeProviderInitializer {
 public:
  TestChromeProviderInitializer();
  virtual ~TestChromeProviderInitializer();

 private:
  std::unique_ptr<TestChromeBrowserProvider> chrome_browser_provider_;

  DISALLOW_COPY_AND_ASSIGN(TestChromeProviderInitializer);
};

}  // namespace ios

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_TEST_CHROME_PROVIDER_INITIALIZER_H_
