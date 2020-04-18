// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/permissions/chooser_context_base.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char* kRequiredKey1 = "key-1";
const char* kRequiredKey2 = "key-2";

class TestChooserContext : public ChooserContextBase {
 public:
  // This class uses the USB content settings type for testing purposes only.
  explicit TestChooserContext(Profile* profile)
      : ChooserContextBase(profile,
                           CONTENT_SETTINGS_TYPE_USB_GUARD,
                           CONTENT_SETTINGS_TYPE_USB_CHOOSER_DATA) {}
  ~TestChooserContext() override {}

  bool IsValidObject(const base::DictionaryValue& object) override {
    return object.size() == 2 && object.HasKey(kRequiredKey1) &&
           object.HasKey(kRequiredKey2);
  }

  std::string GetObjectName(const base::DictionaryValue& object) override {
    NOTREACHED();
    return std::string();
  }
};

}  // namespace

class ChooserContextBaseTest : public testing::Test {
 public:
  ChooserContextBaseTest()
      : origin1_("https://google.com"), origin2_("https://chromium.org") {
    object1_.SetString(kRequiredKey1, "value1");
    object1_.SetString(kRequiredKey2, "value2");
    object2_.SetString(kRequiredKey1, "value3");
    object2_.SetString(kRequiredKey2, "value4");
  }

  ~ChooserContextBaseTest() override {}

  Profile* profile() { return &profile_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;

 protected:
  GURL origin1_;
  GURL origin2_;
  base::DictionaryValue object1_;
  base::DictionaryValue object2_;
};

TEST_F(ChooserContextBaseTest, GrantAndRevokeObjectPermissions) {
  TestChooserContext context(profile());
  context.GrantObjectPermission(origin1_, origin1_, object1_.CreateDeepCopy());
  context.GrantObjectPermission(origin1_, origin1_, object2_.CreateDeepCopy());

  std::vector<std::unique_ptr<base::DictionaryValue>> objects =
      context.GetGrantedObjects(origin1_, origin1_);
  EXPECT_EQ(2u, objects.size());
  EXPECT_TRUE(object1_.Equals(objects[0].get()));
  EXPECT_TRUE(object2_.Equals(objects[1].get()));

  // Granting permission to one origin should not grant them to another.
  objects = context.GetGrantedObjects(origin2_, origin2_);
  EXPECT_EQ(0u, objects.size());

  // Nor when the original origin is embedded in another.
  objects = context.GetGrantedObjects(origin1_, origin2_);
  EXPECT_EQ(0u, objects.size());
}

TEST_F(ChooserContextBaseTest, GrantObjectPermissionTwice) {
  TestChooserContext context(profile());
  context.GrantObjectPermission(origin1_, origin1_, object1_.CreateDeepCopy());
  context.GrantObjectPermission(origin1_, origin1_, object1_.CreateDeepCopy());

  std::vector<std::unique_ptr<base::DictionaryValue>> objects =
      context.GetGrantedObjects(origin1_, origin1_);
  EXPECT_EQ(1u, objects.size());
  EXPECT_TRUE(object1_.Equals(objects[0].get()));

  context.RevokeObjectPermission(origin1_, origin1_, object1_);
  objects = context.GetGrantedObjects(origin1_, origin1_);
  EXPECT_EQ(0u, objects.size());
}

TEST_F(ChooserContextBaseTest, GrantObjectPermissionEmbedded) {
  TestChooserContext context(profile());
  context.GrantObjectPermission(origin1_, origin2_, object1_.CreateDeepCopy());

  std::vector<std::unique_ptr<base::DictionaryValue>> objects =
      context.GetGrantedObjects(origin1_, origin2_);
  EXPECT_EQ(1u, objects.size());
  EXPECT_TRUE(object1_.Equals(objects[0].get()));

  // The embedding origin still does not have permission.
  objects = context.GetGrantedObjects(origin2_, origin2_);
  EXPECT_EQ(0u, objects.size());

  // The requesting origin also doesn't have permission when not embedded.
  objects = context.GetGrantedObjects(origin1_, origin1_);
  EXPECT_EQ(0u, objects.size());
}

TEST_F(ChooserContextBaseTest, GetAllGrantedObjects) {
  TestChooserContext context(profile());
  context.GrantObjectPermission(origin1_, origin1_, object1_.CreateDeepCopy());
  context.GrantObjectPermission(origin2_, origin2_, object2_.CreateDeepCopy());

  std::vector<std::unique_ptr<ChooserContextBase::Object>> objects =
      context.GetAllGrantedObjects();
  EXPECT_EQ(2u, objects.size());
  bool found_one = false;
  bool found_two = false;
  for (const auto& object : objects) {
    if (object->requesting_origin == origin1_) {
      EXPECT_FALSE(found_one);
      EXPECT_EQ(origin1_, object->embedding_origin);
      EXPECT_TRUE(object->object.Equals(&object1_));
      found_one = true;
    } else if (object->requesting_origin == origin2_) {
      EXPECT_FALSE(found_two);
      EXPECT_EQ(origin2_, object->embedding_origin);
      EXPECT_TRUE(object->object.Equals(&object2_));
      found_two = true;
    } else {
      ADD_FAILURE() << "Unexpected object.";
    }
  }
  EXPECT_TRUE(found_one);
  EXPECT_TRUE(found_two);
}

TEST_F(ChooserContextBaseTest, GetGrantedObjectsWithGuardBlocked) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  map->SetContentSettingDefaultScope(origin1_, origin1_,
                                     CONTENT_SETTINGS_TYPE_USB_GUARD,
                                     std::string(), CONTENT_SETTING_BLOCK);

  TestChooserContext context(profile());
  context.GrantObjectPermission(origin1_, origin1_, object1_.CreateDeepCopy());
  context.GrantObjectPermission(origin2_, origin2_, object2_.CreateDeepCopy());

  std::vector<std::unique_ptr<base::DictionaryValue>> objects1 =
      context.GetGrantedObjects(origin1_, origin1_);
  EXPECT_EQ(0u, objects1.size());

  std::vector<std::unique_ptr<base::DictionaryValue>> objects2 =
      context.GetGrantedObjects(origin2_, origin2_);
  ASSERT_EQ(1u, objects2.size());
  EXPECT_TRUE(object2_.Equals(objects2[0].get()));
}

TEST_F(ChooserContextBaseTest, GetAllGrantedObjectsWithGuardBlocked) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  map->SetContentSettingDefaultScope(origin1_, origin1_,
                                     CONTENT_SETTINGS_TYPE_USB_GUARD,
                                     std::string(), CONTENT_SETTING_BLOCK);

  TestChooserContext context(profile());
  context.GrantObjectPermission(origin1_, origin1_, object1_.CreateDeepCopy());
  context.GrantObjectPermission(origin2_, origin2_, object2_.CreateDeepCopy());

  std::vector<std::unique_ptr<ChooserContextBase::Object>> objects =
      context.GetAllGrantedObjects();
  ASSERT_EQ(1u, objects.size());
  EXPECT_EQ(origin2_, objects[0]->requesting_origin);
  EXPECT_EQ(origin2_, objects[0]->embedding_origin);
  EXPECT_TRUE(objects[0]->object.Equals(&object2_));
}
