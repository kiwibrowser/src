// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/extension_process_policy.h"

#include "components/crx_file/id_util.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

scoped_refptr<const Extension> CreateExtension(const std::string& id_seed) {
  std::unique_ptr<base::DictionaryValue> manifest =
      DictionaryBuilder()
          .Set("name", "test")
          .Set("version", "1.0")
          .Set("manifest_version", 2)
          .Build();
  return ExtensionBuilder()
      .SetManifest(std::move(manifest))
      .SetID(crx_file::id_util::GenerateId(id_seed))
      .Build();
}

}  // namespace

TEST(CrossesExtensionBoundaryTest, InstalledExtensions) {
  ExtensionSet extensions;
  scoped_refptr<const Extension> extension1 = CreateExtension("a");
  scoped_refptr<const Extension> extension2 = CreateExtension("b");
  extensions.Insert(extension1);
  extensions.Insert(extension2);

  GURL web_url("https://example.com");

  EXPECT_TRUE(CrossesExtensionProcessBoundary(extensions, web_url,
                                              extension1->url(), false));
  EXPECT_TRUE(CrossesExtensionProcessBoundary(extensions, extension1->url(),
                                              extension2->url(), false));
  EXPECT_TRUE(CrossesExtensionProcessBoundary(extensions, extension1->url(),
                                              web_url, false));
}

TEST(CrossesExtensionBoundaryTest, UninstalledExtensions) {
  ExtensionSet extensions;
  scoped_refptr<const Extension> extension1 = CreateExtension("a");
  extensions.Insert(extension1);
  GURL web_url("https://example.com");
  GURL non_existent_extension_url("chrome-extension://" + std::string(32, 'a') +
                                  "/foo");

  EXPECT_TRUE(CrossesExtensionProcessBoundary(
      extensions, web_url, non_existent_extension_url, false));
  EXPECT_TRUE(CrossesExtensionProcessBoundary(
      extensions, extension1->url(), non_existent_extension_url, false));
}

}  // namespace extensions
