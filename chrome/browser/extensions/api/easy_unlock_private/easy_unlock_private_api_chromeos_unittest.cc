// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/easy_unlock_private/easy_unlock_private_api.h"

#include <memory>
#include <utility>

#include "chrome/browser/extensions/api/easy_unlock_private/easy_unlock_private_connection_manager.h"
#include "chrome/browser/extensions/extension_api_unittest.h"
#include "components/cryptauth/fake_connection.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/value_builder.h"

namespace extensions {
namespace {

using cryptauth::FakeConnection;
using cryptauth::CreateRemoteDeviceForTest;

using extensions::BrowserContextKeyedAPIFactory;
using extensions::DictionaryBuilder;
using extensions::Extension;
using extensions::ExtensionBuilder;
using extensions::ListBuilder;

EasyUnlockPrivateConnectionManager* GetConnectionManager(
    content::BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<EasyUnlockPrivateAPI>::Get(context)
      ->get_connection_manager();
}

scoped_refptr<const Extension> CreateTestExtension() {
  return ExtensionBuilder()
      .SetManifest(
          DictionaryBuilder()
              .Set("name", "Extension")
              .Set("version", "1.0")
              .Set("manifest_version", 2)
              .Set("permissions", ListBuilder().Append("<all_urls>").Build())
              .Build())
      .SetID("test")
      .Build();
}

class EasyUnlockPrivateApiTest : public extensions::ExtensionApiUnittest {
 public:
  EasyUnlockPrivateApiTest() {}
  ~EasyUnlockPrivateApiTest() override {}
};

// Tests that no BrowserContext dependencies of EasyUnlockPrivateApi (and its
// dependencies) are referenced after the BrowserContext is torn down. The test
// fails with a crash if such a condition exists.
TEST_F(EasyUnlockPrivateApiTest, BrowserContextTearDown) {
  auto* manager = GetConnectionManager(profile());
  ASSERT_TRUE(!!manager);

  // Add a Connection. The shutdown path for EasyUnlockPrivateConnectionManager,
  // a dependency of EasyUnlockPrivateApi, only references BrowserContext
  // dependencies if it has a Connection to shutdown.
  auto extension = CreateTestExtension();
  auto connection = std::make_unique<FakeConnection>(
      cryptauth::CreateRemoteDeviceRefForTest());
  manager->AddConnection(extension.get(), std::move(connection), true);

  // The Profile is cleaned up at the end of this scope, and BrowserContext
  // shutdown logic asserts no browser dependencies are referenced afterward.
}

}  // namespace
}  // namespace extensions
