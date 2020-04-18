// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "chrome/browser/extensions/extension_prefs_unittest.h"
#include "chrome/test/base/testing_profile.h"
#include "extensions/common/extension.h"

namespace extensions {

// Tests force hiding browser actions.
class ExtensionPrefsHidingBrowserActions : public ExtensionPrefsTest {
 public:
  ExtensionPrefsHidingBrowserActions() {}
  ~ExtensionPrefsHidingBrowserActions() override {}

  void Initialize() override {
    profile_.reset(new TestingProfile());

    // Install 5 extensions.
    for (int i = 0; i < 5; i++) {
      std::string name = "test" + base::IntToString(i);
      extensions_.push_back(prefs_.AddExtension(name));
    }

    ExtensionActionAPI* action_api = ExtensionActionAPI::Get(profile_.get());
    action_api->set_prefs_for_testing(prefs());
    for (const scoped_refptr<const Extension>& extension : extensions_)
      EXPECT_TRUE(action_api->GetBrowserActionVisibility(extension->id()));

    action_api->SetBrowserActionVisibility(extensions_[0]->id(), false);
    action_api->SetBrowserActionVisibility(extensions_[1]->id(), true);
  }

  void Verify() override {
    ExtensionActionAPI* action_api = ExtensionActionAPI::Get(profile_.get());
    action_api->set_prefs_for_testing(prefs());
    // Make sure the one we hid is hidden.
    EXPECT_FALSE(action_api->GetBrowserActionVisibility(extensions_[0]->id()));

    // Make sure the other id's are not hidden.
    ExtensionList::const_iterator iter = extensions_.begin() + 1;
    for (; iter != extensions_.end(); ++iter) {
      SCOPED_TRACE(base::StringPrintf("Loop %d ",
                       static_cast<int>(iter - extensions_.begin())));
      EXPECT_TRUE(action_api->GetBrowserActionVisibility((*iter)->id()));
    }
  }

 private:
  std::unique_ptr<TestingProfile> profile_;
  ExtensionList extensions_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionPrefsHidingBrowserActions);
};

TEST_F(ExtensionPrefsHidingBrowserActions, ForceHide) {}

}  // namespace extensions
