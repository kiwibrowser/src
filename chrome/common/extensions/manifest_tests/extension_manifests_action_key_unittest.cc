// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/values_test_util.h"
#include "chrome/common/extensions/manifest_tests/chrome_manifest_test.h"
#include "components/version_info/version_info.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

class ActionKeyTest : public ManifestTest {
 protected:
  ManifestData CreateManifest(const std::string& action_json) {
    std::unique_ptr<base::DictionaryValue> manifest =
        base::DictionaryValue::From(base::test::ParseJson(R"json({
                                    "name": "test",
                                    "version": "1",
                                    "manifest_version": 2, )json" +
                                                          action_json + "}"));
    EXPECT_TRUE(manifest);
    return ManifestData(std::move(manifest), "test");
  }
};

TEST_F(ActionKeyTest, InvalidType) {
  // TODO(catmullings): When ready, remove trunk channel restriction.
  extensions::ScopedCurrentChannel scoped_channel(
      version_info::Channel::UNKNOWN);

  LoadAndExpectError(CreateManifest(
                         R"("action": {
                               "default_state": "invalid"
                             })"),
                     manifest_errors::kInvalidActionDefaultState);
  LoadAndExpectError(CreateManifest(
                         R"("page_action": {
                               "default_state": "enabled"
                             })"),
                     manifest_errors::kDefaultStateShouldNotBeSet);
  LoadAndExpectError(CreateManifest(
                         R"("page_action": {
                               "default_state": "invalid"
                             })"),
                     manifest_errors::kInvalidActionDefaultState);
  LoadAndExpectError(CreateManifest(
                         R"("browser_action": {
                               "default_state": "enabled"
                             })"),
                     manifest_errors::kDefaultStateShouldNotBeSet);
}

}  // namespace extensions
