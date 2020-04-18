// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/networking_config/networking_config_service.h"

#include <utility>

#include "extensions/browser/api_unittest.h"
#include "extensions/browser/extension_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

const char kExtensionId[] = "necdpnkfgondfageiompbacibhgmfebg";
const char kHexSsid[] = "54657374535349445F5A5A5A5A";
const char kHexSsidLower[] = "54657374535349445f5a5a5a5a";

class MockEventDelegate : public NetworkingConfigService::EventDelegate {
 public:
  MockEventDelegate() : extension_registered_(false) {}
  ~MockEventDelegate() override {}

  bool HasExtensionRegisteredForEvent(
      const std::string& extension_id) const override {
    return extension_registered_;
  }

  void SetExtensionRegisteredForEvent(bool extension_registered) {
    extension_registered_ = extension_registered;
  }

 private:
  bool extension_registered_;
};

}  // namespace

class NetworkingConfigServiceTest : public ApiUnitTest {
 public:
  NetworkingConfigServiceTest() {}
  ~NetworkingConfigServiceTest() override {}

  void SetUp() override {
    ApiUnitTest::SetUp();
    extension_registry_ = std::unique_ptr<ExtensionRegistry>(
        new ExtensionRegistry(browser_context()));
    std::unique_ptr<MockEventDelegate> mock_event_delegate =
        std::unique_ptr<MockEventDelegate>(new MockEventDelegate());
    service_ =
        std::unique_ptr<NetworkingConfigService>(new NetworkingConfigService(
            browser_context(), std::move(mock_event_delegate),
            extension_registry_.get()));
    DCHECK(service_);
  }

 protected:
  std::unique_ptr<ExtensionRegistry> extension_registry_;
  std::unique_ptr<NetworkingConfigService> service_;
};

TEST_F(NetworkingConfigServiceTest, BasicRegisterHexSsid) {
  EXPECT_TRUE(service_->RegisterHexSsid(kHexSsid, kExtensionId));
  EXPECT_EQ(kExtensionId, service_->LookupExtensionIdForHexSsid(kHexSsid));
  EXPECT_EQ(kExtensionId, service_->LookupExtensionIdForHexSsid(kHexSsidLower));
}

TEST_F(NetworkingConfigServiceTest, BasicRegisterHexSsidLower) {
  EXPECT_TRUE(service_->RegisterHexSsid(kHexSsidLower, kExtensionId));
  EXPECT_EQ(kExtensionId, service_->LookupExtensionIdForHexSsid(kHexSsid));
  EXPECT_EQ(kExtensionId, service_->LookupExtensionIdForHexSsid(kHexSsidLower));
}

}  // namespace extensions
