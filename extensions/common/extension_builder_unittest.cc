// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/extension_builder.h"

#include "base/version.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/manifest_handlers/externally_connectable.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

TEST(ExtensionBuilderTest, Basic) {
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("some name").Build();
    ASSERT_TRUE(extension);
    EXPECT_EQ("some name", extension->name());
    EXPECT_TRUE(extension->is_extension());
    EXPECT_EQ(2, extension->manifest_version());
  }
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("some app", ExtensionBuilder::Type::PLATFORM_APP)
            .Build();
    ASSERT_TRUE(extension);
    EXPECT_EQ("some app", extension->name());
    EXPECT_TRUE(extension->is_platform_app());
    EXPECT_EQ(2, extension->manifest_version());
  }
}

TEST(ExtensionBuilderTest, Permissions) {
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("no permissions").Build();
    EXPECT_TRUE(extension->permissions_data()->active_permissions().IsEmpty());
  }
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("permissions")
            .AddPermission("storage")
            .AddPermissions({"alarms", "idle"})
            .Build();
    EXPECT_TRUE(extension->permissions_data()->HasAPIPermission("storage"));
    EXPECT_TRUE(extension->permissions_data()->HasAPIPermission("alarms"));
    EXPECT_TRUE(extension->permissions_data()->HasAPIPermission("idle"));
  }
}

TEST(ExtensionBuilderTest, Actions) {
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("no action").Build();
    EXPECT_FALSE(extension->manifest()->HasKey(manifest_keys::kPageAction));
    EXPECT_FALSE(extension->manifest()->HasKey(manifest_keys::kBrowserAction));
  }
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("page action")
            .SetAction(ExtensionBuilder::ActionType::PAGE_ACTION)
            .Build();
    EXPECT_TRUE(extension->manifest()->HasKey(manifest_keys::kPageAction));
    EXPECT_FALSE(extension->manifest()->HasKey(manifest_keys::kBrowserAction));
  }
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("browser action")
            .SetAction(ExtensionBuilder::ActionType::BROWSER_ACTION)
            .Build();
    EXPECT_FALSE(extension->manifest()->HasKey(manifest_keys::kPageAction));
    EXPECT_TRUE(extension->manifest()->HasKey(manifest_keys::kBrowserAction));
  }
}

TEST(ExtensionBuilderTest, Background) {
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("no background").Build();
    EXPECT_FALSE(BackgroundInfo::HasBackgroundPage(extension.get()));
  }
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("persistent background page")
            .SetBackgroundPage(ExtensionBuilder::BackgroundPage::PERSISTENT)
            .Build();
    EXPECT_TRUE(BackgroundInfo::HasBackgroundPage(extension.get()));
    EXPECT_TRUE(BackgroundInfo::HasPersistentBackgroundPage(extension.get()));
  }
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("event page")
            .SetBackgroundPage(ExtensionBuilder::BackgroundPage::EVENT)
            .Build();
    EXPECT_TRUE(BackgroundInfo::HasBackgroundPage(extension.get()));
    EXPECT_TRUE(BackgroundInfo::HasLazyBackgroundPage(extension.get()));
  }
}

TEST(ExtensionBuilderTest, MergeManifest) {
  DictionaryBuilder connectable;
  connectable.Set("matches", ListBuilder().Append("*://example.com/*").Build());
  std::unique_ptr<base::DictionaryValue> connectable_value =
      DictionaryBuilder()
          .Set("externally_connectable", connectable.Build())
          .Build();
  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extra")
          .MergeManifest(std::move(connectable_value))
          .Build();
  EXPECT_TRUE(ExternallyConnectableInfo::Get(extension.get()));
}

TEST(ExtensionBuilderTest, IDUniqueness) {
  scoped_refptr<const Extension> a = ExtensionBuilder("a").Build();
  scoped_refptr<const Extension> b = ExtensionBuilder("b").Build();
  scoped_refptr<const Extension> c = ExtensionBuilder("c").Build();

  std::set<ExtensionId> ids = {a->id(), b->id(), c->id()};
  EXPECT_EQ(3u, ids.size());
}

TEST(ExtensionBuilderTest, SetManifestAndMergeManifest) {
  DictionaryBuilder manifest;
  manifest.Set("name", "some name")
      .Set("manifest_version", 2)
      .Set("description", "some description");
  scoped_refptr<const Extension> extension =
      ExtensionBuilder()
          .SetManifest(manifest.Build())
          .MergeManifest(DictionaryBuilder().Set("version", "0.1").Build())
          .Build();
  EXPECT_EQ("some name", extension->name());
  EXPECT_EQ(2, extension->manifest_version());
  EXPECT_EQ("some description", extension->description());
  EXPECT_EQ("0.1", extension->version().GetString());
}

TEST(ExtensionBuilderTest, MergeManifestOverridesValues) {
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("foo")
            .MergeManifest(DictionaryBuilder().Set("version", "52.0.9").Build())
            .Build();
    // MergeManifest() should have overwritten the default 0.1 value for
    // version.
    EXPECT_EQ("52.0.9", extension->version().GetString());
  }

  {
    DictionaryBuilder manifest;
    manifest.Set("name", "some name")
        .Set("manifest_version", 2)
        .Set("description", "some description")
        .Set("version", "0.1");
    scoped_refptr<const Extension> extension =
        ExtensionBuilder()
            .SetManifest(manifest.Build())
            .MergeManifest(DictionaryBuilder().Set("version", "42.1").Build())
            .Build();
    EXPECT_EQ("42.1", extension->version().GetString());
  }
}

TEST(ExtensionBuilderTest, SetManifestKey) {
  scoped_refptr<const Extension> extension =
      ExtensionBuilder("foo")
          .SetManifestKey("short_name", "short name")
          .Build();
  EXPECT_EQ("short name", extension->short_name());
}

}  // namespace extensions
