// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/location_bar/location_bar.h"

#include <memory>

#include "base/macros.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/value_builder.h"

class LocationBarBrowserTest : public extensions::ExtensionBrowserTest {
 public:
  LocationBarBrowserTest() {}
  ~LocationBarBrowserTest() override {}

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override;

 private:
  std::unique_ptr<extensions::FeatureSwitch::ScopedOverride> enable_override_;

  DISALLOW_COPY_AND_ASSIGN(LocationBarBrowserTest);
};

void LocationBarBrowserTest::SetUpCommandLine(base::CommandLine* command_line) {
  extensions::ExtensionBrowserTest::SetUpCommandLine(command_line);

  // In order to let a vanilla extension override the bookmark star, we have to
  // enable the switch.
  enable_override_ =
      std::make_unique<extensions::FeatureSwitch::ScopedOverride>(
          extensions::FeatureSwitch::enable_override_bookmarks_ui(), true);
}

// Test that installing an extension that overrides the bookmark star
// successfully hides the star.
IN_PROC_BROWSER_TEST_F(LocationBarBrowserTest,
                       ExtensionCanOverrideBookmarkStar) {
  LocationBarTesting* location_bar =
      browser()->window()->GetLocationBar()->GetLocationBarForTesting();
  // By default, we should show the star.
  EXPECT_TRUE(location_bar->GetBookmarkStarVisibility());

  // Create and install an extension that overrides the bookmark star.
  extensions::DictionaryBuilder chrome_ui_overrides;
  chrome_ui_overrides.Set(
      "bookmarks_ui",
      extensions::DictionaryBuilder().Set("remove_button", true).Build());
  scoped_refptr<const extensions::Extension> extension =
      extensions::ExtensionBuilder()
          .SetManifest(
              extensions::DictionaryBuilder()
                  .Set("name", "overrides star")
                  .Set("manifest_version", 2)
                  .Set("version", "0.1")
                  .Set("description", "override the star")
                  .Set("chrome_ui_overrides", chrome_ui_overrides.Build())
                  .Build())
          .Build();
  extension_service()->AddExtension(extension.get());

  // The star should now be hidden.
  EXPECT_FALSE(location_bar->GetBookmarkStarVisibility());
}
