// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/product.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_reg_util_win.h"
#include "base/version.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/installation_state.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::win::RegKey;
using installer::Product;
using registry_util::RegistryOverrideManager;

TEST(ProductTest, ProductInstallBasic) {
  // TODO(tommi): We should mock this and use our mocked distribution.
  const bool system_level = true;
  installer::InstallationState machine_state;
  machine_state.Initialize();

  std::unique_ptr<Product> product =
      std::make_unique<Product>(BrowserDistribution::GetDistribution());
  BrowserDistribution* distribution = product->distribution();

  base::FilePath user_data_dir;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir));
  EXPECT_FALSE(user_data_dir.empty());

  base::FilePath program_files;
  ASSERT_TRUE(base::PathService::Get(base::DIR_PROGRAM_FILES, &program_files));
  // The User Data path should never be under program files, even though
  // system_level is true.
  EXPECT_EQ(std::wstring::npos,
            user_data_dir.value().find(program_files.value()));

  HKEY root = system_level ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  {
    RegistryOverrideManager override_manager;
    ASSERT_NO_FATAL_FAILURE(override_manager.OverrideRegistry(root));

    // There should be no installed version in the registry.
    machine_state.Initialize();
    EXPECT_EQ(nullptr, machine_state.GetProductState(system_level));

    // Let's pretend chrome is installed.
    RegKey version_key(root, distribution->GetVersionKey().c_str(),
                       KEY_ALL_ACCESS);
    ASSERT_TRUE(version_key.Valid());

    const char kCurrentVersion[] = "1.2.3.4";
    base::Version current_version(kCurrentVersion);
    version_key.WriteValue(google_update::kRegVersionField,
                           base::UTF8ToWide(
                               current_version.GetString()).c_str());

    // We started out with a non-msi product.
    machine_state.Initialize();
    const installer::ProductState* chrome_state =
        machine_state.GetProductState(system_level);
    EXPECT_NE(nullptr, chrome_state);
    if (chrome_state) {
      EXPECT_EQ(chrome_state->version(), current_version);
      EXPECT_FALSE(chrome_state->is_msi());
    }

    // Create a make-believe client state key.
    RegKey key;
    std::wstring state_key_path(distribution->GetStateKey());
    ASSERT_EQ(ERROR_SUCCESS,
        key.Create(root, state_key_path.c_str(), KEY_ALL_ACCESS));

    // Set the MSI marker, refresh, and verify that we now see the MSI marker.
    EXPECT_TRUE(product->SetMsiMarker(system_level, true));
    machine_state.Initialize();
    chrome_state = machine_state.GetProductState(system_level);
    EXPECT_NE(nullptr, chrome_state);
    if (chrome_state)
      EXPECT_TRUE(chrome_state->is_msi());
  }
}

TEST(ProductTest, LaunchChrome) {
  // TODO(tommi): Test Product::LaunchChrome and
  // Product::LaunchChromeAndWait.
  NOTIMPLEMENTED();
}
