// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/common/extensions/manifest_tests/chrome_manifest_test.h"
#include "chrome/common/url_constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace errors = manifest_errors;

typedef ChromeManifestTest ChromePermissionManifestTest;

TEST_F(ChromePermissionManifestTest, ChromeURLPermissionInvalid) {
  LoadAndExpectWarning("permission_chrome_url_invalid.json",
                       ErrorUtils::FormatErrorMessage(
                           errors::kInvalidPermissionScheme,
                           chrome::kChromeUINewTabURL));
}

TEST_F(ChromePermissionManifestTest, ChromeURLPermissionAllowedWithFlag) {
  // Ignore the policy delegate for this test.
  PermissionsData::SetPolicyDelegate(NULL);
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kExtensionsOnChromeURLs);
  std::string error;
  scoped_refptr<Extension> extension =
    LoadAndExpectSuccess("permission_chrome_url_invalid.json");
  EXPECT_EQ("", error);
  const GURL newtab_url(chrome::kChromeUINewTabURL);
  EXPECT_TRUE(
      extension->permissions_data()->CanAccessPage(newtab_url, 0, &error))
      << error;
}

TEST_F(ChromePermissionManifestTest,
       ChromeResourcesPermissionValidOnlyForComponents) {
  LoadAndExpectWarning("permission_chrome_resources_url.json",
                       ErrorUtils::FormatErrorMessage(
                           errors::kInvalidPermissionScheme,
                           "chrome://resources/"));
  std::string error;
  LoadExtension(ManifestData("permission_chrome_resources_url.json"),
                &error,
                extensions::Manifest::COMPONENT,
                Extension::NO_FLAGS);
  EXPECT_EQ("", error);
}

}  // namespace extensions
