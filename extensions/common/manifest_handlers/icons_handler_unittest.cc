// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/values_test_util.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "extensions/common/manifest_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

class ProductIconManifestTest : public ManifestTest {
 public:
  ProductIconManifestTest() {}

 protected:
  std::unique_ptr<base::DictionaryValue> CreateManifest(
      const std::string& extra_icons) {
    std::unique_ptr<base::DictionaryValue> manifest =
        base::DictionaryValue::From(
            base::test::ParseJson("{ \n"
                                  "  \"name\": \"test\", \n"
                                  "  \"version\": \"0.1\", \n"
                                  "  \"manifest_version\": 2, \n"
                                  "  \"icons\": { \n" +
                                  extra_icons + "    \"16\": \"icon1.png\", \n"
                                                "    \"32\": \"icon2.png\" \n"
                                                "  } \n"
                                                "} \n"));
    EXPECT_TRUE(manifest);
    return manifest;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ProductIconManifestTest);
};

TEST_F(ProductIconManifestTest, Sizes) {
  // Too big.
  {
    std::unique_ptr<base::DictionaryValue> ext_manifest =
        CreateManifest("\"100000\": \"icon3.png\", \n");
    ManifestData manifest(std::move(ext_manifest), "test");
    LoadAndExpectError(manifest, "Invalid key in icons: \"100000\".");
  }
  // Too small.
  {
    std::unique_ptr<base::DictionaryValue> ext_manifest =
        CreateManifest("\"0\": \"icon3.png\", \n");
    ManifestData manifest(std::move(ext_manifest), "test");
    LoadAndExpectError(manifest, "Invalid key in icons: \"0\".");
  }
  // NaN.
  {
    std::unique_ptr<base::DictionaryValue> ext_manifest =
        CreateManifest("\"sixteen\": \"icon3.png\", \n");
    ManifestData manifest(std::move(ext_manifest), "test");
    LoadAndExpectError(manifest, "Invalid key in icons: \"sixteen\".");
  }
  // Just right.
  {
    std::unique_ptr<base::DictionaryValue> ext_manifest =
        CreateManifest("\"512\": \"icon3.png\", \n");
    ManifestData manifest(std::move(ext_manifest), "test");
    scoped_refptr<extensions::Extension> extension =
        LoadAndExpectSuccess(manifest);
  }
}

}  // namespace extensions
