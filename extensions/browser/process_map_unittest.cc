// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/process_map.h"

#include "base/memory/ref_counted.h"
#include "base/values.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/features/feature.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace {

enum class TypeToCreate { kExtension, kHostedApp, kPlatformApp };

scoped_refptr<const Extension> CreateExtensionWithFlags(TypeToCreate type,
                                                        const std::string& id) {
  DictionaryBuilder manifest_builder;
  manifest_builder.Set("name", "Test extension")
      .Set("version", "1.0")
      .Set("manifest_version", 2);

  switch (type) {
    case TypeToCreate::kExtension:
      manifest_builder.Set(
          "background",
          DictionaryBuilder()
              .Set("scripts", ListBuilder().Append("background.js").Build())
              .Build());
      break;
    case TypeToCreate::kHostedApp:
      manifest_builder.Set(
          "app", DictionaryBuilder()
                     .Set("launch", DictionaryBuilder()
                                        .Set("web_url", "https://www.foo.bar")
                                        .Build())
                     .Build());
      break;
    case TypeToCreate::kPlatformApp:
      manifest_builder.Set(
          "app",
          DictionaryBuilder()
              .Set("background",
                   DictionaryBuilder()
                       .Set("scripts",
                            ListBuilder().Append("background.js").Build())
                       .Build())
              .Build());
      break;
  }

  return ExtensionBuilder()
      .SetID(id)
      .SetManifest(manifest_builder.Build())
      .Build();
}

}  // namespace
}  // namespace extensions

using extensions::ProcessMap;

TEST(ExtensionProcessMapTest, Test) {
  ProcessMap map;

  // Test behavior when empty.
  EXPECT_FALSE(map.Contains("a", 1));
  EXPECT_FALSE(map.Remove("a", 1, 1));
  EXPECT_EQ(0u, map.size());

  // Test insertion and behavior with one item.
  EXPECT_TRUE(map.Insert("a", 1, 1));
  EXPECT_TRUE(map.Contains("a", 1));
  EXPECT_FALSE(map.Contains("a", 2));
  EXPECT_FALSE(map.Contains("b", 1));
  EXPECT_EQ(1u, map.size());

  // Test inserting a duplicate item.
  EXPECT_FALSE(map.Insert("a", 1, 1));
  EXPECT_TRUE(map.Contains("a", 1));
  EXPECT_EQ(1u, map.size());

  // Insert some more items.
  EXPECT_TRUE(map.Insert("a", 2, 2));
  EXPECT_TRUE(map.Insert("b", 1, 3));
  EXPECT_TRUE(map.Insert("b", 2, 4));
  EXPECT_EQ(4u, map.size());

  EXPECT_TRUE(map.Contains("a", 1));
  EXPECT_TRUE(map.Contains("a", 2));
  EXPECT_TRUE(map.Contains("b", 1));
  EXPECT_TRUE(map.Contains("b", 2));
  EXPECT_FALSE(map.Contains("a", 3));

  // Note that this only differs from an existing item because of the site
  // instance id.
  EXPECT_TRUE(map.Insert("a", 1, 5));
  EXPECT_TRUE(map.Contains("a", 1));

  // Test removal.
  EXPECT_TRUE(map.Remove("a", 1, 1));
  EXPECT_FALSE(map.Remove("a", 1, 1));
  EXPECT_EQ(4u, map.size());

  // Should still return true because there were two site instances for this
  // extension/process pair.
  EXPECT_TRUE(map.Contains("a", 1));

  EXPECT_TRUE(map.Remove("a", 1, 5));
  EXPECT_EQ(3u, map.size());
  EXPECT_FALSE(map.Contains("a", 1));

  EXPECT_EQ(2, map.RemoveAllFromProcess(2));
  EXPECT_EQ(1u, map.size());
  EXPECT_EQ(0, map.RemoveAllFromProcess(2));
  EXPECT_EQ(1u, map.size());
}

TEST(ExtensionProcessMapTest, GetMostLikelyContextType) {
  ProcessMap map;

  EXPECT_EQ(extensions::Feature::WEB_PAGE_CONTEXT,
            map.GetMostLikelyContextType(nullptr, 1));

  scoped_refptr<const extensions::Extension> extension =
      CreateExtensionWithFlags(extensions::TypeToCreate::kExtension, "a");

  EXPECT_EQ(extensions::Feature::CONTENT_SCRIPT_CONTEXT,
            map.GetMostLikelyContextType(extension.get(), 2));

  map.Insert("b", 3, 1);
  extension =
      CreateExtensionWithFlags(extensions::TypeToCreate::kExtension, "b");
  EXPECT_EQ(extensions::Feature::BLESSED_EXTENSION_CONTEXT,
            map.GetMostLikelyContextType(extension.get(), 3));

  map.Insert("c", 4, 2);
  extension =
      CreateExtensionWithFlags(extensions::TypeToCreate::kPlatformApp, "c");
  EXPECT_EQ(extensions::Feature::BLESSED_EXTENSION_CONTEXT,
            map.GetMostLikelyContextType(extension.get(), 4));

  map.set_is_lock_screen_context(true);

  map.Insert("d", 5, 3);
  extension =
      CreateExtensionWithFlags(extensions::TypeToCreate::kPlatformApp, "d");
  EXPECT_EQ(extensions::Feature::LOCK_SCREEN_EXTENSION_CONTEXT,
            map.GetMostLikelyContextType(extension.get(), 5));

  map.Insert("e", 6, 4);
  extension =
      CreateExtensionWithFlags(extensions::TypeToCreate::kExtension, "e");
  EXPECT_EQ(extensions::Feature::LOCK_SCREEN_EXTENSION_CONTEXT,
            map.GetMostLikelyContextType(extension.get(), 6));

  map.Insert("f", 7, 5);
  extension =
      CreateExtensionWithFlags(extensions::TypeToCreate::kHostedApp, "f");
  EXPECT_EQ(extensions::Feature::BLESSED_WEB_PAGE_CONTEXT,
            map.GetMostLikelyContextType(extension.get(), 7));
}
