// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_tab_util.h"

#include "base/macros.h"
#include "chrome/common/extensions/api/tabs.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

const char kCustomUrl[] = "www.example.com/foo?bar=baz";

class ExtensionTabUtilTestDelegate : public ExtensionTabUtil::Delegate {
 public:
  ExtensionTabUtilTestDelegate() {}
  ~ExtensionTabUtilTestDelegate() override {}

  // ExtensionTabUtil::Delegate
  void ScrubTabForExtension(const Extension* extension,
                            content::WebContents* contents,
                            api::tabs::Tab* tab) override {
    tab->url.reset(new std::string(kCustomUrl));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtensionTabUtilTestDelegate);
};

}  // namespace

// Test that the custom ScrubTabForExtension delegate works - in this test it
// sets URL to a custom string.
TEST(ExtensionTabUtilTest, Delegate) {
  auto test_delegate = std::make_unique<ExtensionTabUtilTestDelegate>();
  ExtensionTabUtil::SetPlatformDelegate(test_delegate.get());

  api::tabs::Tab tab;
  ExtensionTabUtil::ScrubTabForExtension(nullptr, nullptr, &tab);
  EXPECT_EQ(kCustomUrl, *tab.url);

  // Unset the delegate.
  ExtensionTabUtil::SetPlatformDelegate(nullptr);
}

}  // namespace extensions
