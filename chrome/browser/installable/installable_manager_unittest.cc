// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/installable_manager.h"

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/manifest/web_display_mode.h"

using IconPurpose = blink::Manifest::Icon::IconPurpose;

class InstallableManagerUnitTest : public testing::Test {
 public:
  InstallableManagerUnitTest()
      : manager_(std::make_unique<InstallableManager>(nullptr)) {}

 protected:
  static base::NullableString16 ToNullableUTF16(const std::string& str) {
    return base::NullableString16(base::UTF8ToUTF16(str), false);
  }

  static blink::Manifest GetValidManifest() {
    blink::Manifest manifest;
    manifest.name = ToNullableUTF16("foo");
    manifest.short_name = ToNullableUTF16("bar");
    manifest.start_url = GURL("http://example.com");
    manifest.display = blink::kWebDisplayModeStandalone;

    blink::Manifest::Icon primary_icon;
    primary_icon.type = base::ASCIIToUTF16("image/png");
    primary_icon.sizes.push_back(gfx::Size(144, 144));
    primary_icon.purpose.push_back(IconPurpose::ANY);
    manifest.icons.push_back(primary_icon);

    // No need to include the optional badge icon as it does not affect the
    // unit tests.
    return manifest;
  }

  bool IsManifestValid(const blink::Manifest& manifest) {
    // Explicitly reset the error code before running the method.
    manager_->set_valid_manifest_error(NO_ERROR_DETECTED);
    return manager_->IsManifestValidForWebApp(manifest);
  }

  InstallableStatusCode GetErrorCode() {
    return manager_->valid_manifest_error();
  }

 private:
  std::unique_ptr<InstallableManager> manager_;
};

TEST_F(InstallableManagerUnitTest, EmptyManifestIsInvalid) {
  blink::Manifest manifest;
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_EMPTY, GetErrorCode());
}

TEST_F(InstallableManagerUnitTest, CheckMinimalValidManifest) {
  blink::Manifest manifest = GetValidManifest();
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());
}

TEST_F(InstallableManagerUnitTest, ManifestRequiresNameOrShortName) {
  blink::Manifest manifest = GetValidManifest();

  manifest.name = base::NullableString16();
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  manifest.name = ToNullableUTF16("foo");
  manifest.short_name = base::NullableString16();
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  manifest.name = base::NullableString16();
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_MISSING_NAME_OR_SHORT_NAME, GetErrorCode());
}

TEST_F(InstallableManagerUnitTest, ManifestRequiresNonEmptyNameORShortName) {
  blink::Manifest manifest = GetValidManifest();

  manifest.name = ToNullableUTF16("");
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  manifest.name = ToNullableUTF16("foo");
  manifest.short_name = ToNullableUTF16("");
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  manifest.name = ToNullableUTF16("");
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_MISSING_NAME_OR_SHORT_NAME, GetErrorCode());
}

TEST_F(InstallableManagerUnitTest, ManifestRequiresValidStartURL) {
  blink::Manifest manifest = GetValidManifest();

  manifest.start_url = GURL();
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(START_URL_NOT_VALID, GetErrorCode());

  manifest.start_url = GURL("/");
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(START_URL_NOT_VALID, GetErrorCode());
}

TEST_F(InstallableManagerUnitTest, ManifestRequiresImagePNG) {
  blink::Manifest manifest = GetValidManifest();

  manifest.icons[0].type = base::ASCIIToUTF16("image/gif");
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_MISSING_SUITABLE_ICON, GetErrorCode());

  manifest.icons[0].type.clear();
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_MISSING_SUITABLE_ICON, GetErrorCode());

  // If the type is null, the icon src will be checked instead.
  manifest.icons[0].src = GURL("http://example.com/icon.png");
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  // Capital file extension is also permissible.
  manifest.icons[0].src = GURL("http://example.com/icon.PNG");
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  // Non-png extensions are rejected.
  manifest.icons[0].src = GURL("http://example.com/icon.gif");
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_MISSING_SUITABLE_ICON, GetErrorCode());
}

TEST_F(InstallableManagerUnitTest, ManifestRequiresPurposeAny) {
  blink::Manifest manifest = GetValidManifest();

  // The icon MUST have IconPurpose::ANY at least.
  manifest.icons[0].purpose[0] = IconPurpose::BADGE;
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_MISSING_SUITABLE_ICON, GetErrorCode());

  // If one of the icon purposes match the requirement, it should be accepted.
  manifest.icons[0].purpose.push_back(IconPurpose::ANY);
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());
}

TEST_F(InstallableManagerUnitTest, ManifestRequiresMinimalSize) {
  blink::Manifest manifest = GetValidManifest();

  // The icon MUST be 144x144 size at least.
  manifest.icons[0].sizes[0] = gfx::Size(1, 1);
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_MISSING_SUITABLE_ICON, GetErrorCode());

  manifest.icons[0].sizes[0] = gfx::Size(143, 143);
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_MISSING_SUITABLE_ICON, GetErrorCode());

  // If one of the sizes match the requirement, it should be accepted.
  manifest.icons[0].sizes.push_back(gfx::Size(144, 144));
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  // Higher than the required size is okay.
  manifest.icons[0].sizes[1] = gfx::Size(200, 200);
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  // Non-square is okay.
  manifest.icons[0].sizes[1] = gfx::Size(144, 200);
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  // The representation of the keyword 'any' should be recognized.
  manifest.icons[0].sizes[1] = gfx::Size(0, 0);
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());
}

TEST_F(InstallableManagerUnitTest, ManifestDisplayModes) {
  blink::Manifest manifest = GetValidManifest();

  manifest.display = blink::kWebDisplayModeUndefined;
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_DISPLAY_NOT_SUPPORTED, GetErrorCode());

  manifest.display = blink::kWebDisplayModeBrowser;
  EXPECT_FALSE(IsManifestValid(manifest));
  EXPECT_EQ(MANIFEST_DISPLAY_NOT_SUPPORTED, GetErrorCode());

  manifest.display = blink::kWebDisplayModeMinimalUi;
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  manifest.display = blink::kWebDisplayModeStandalone;
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());

  manifest.display = blink::kWebDisplayModeFullscreen;
  EXPECT_TRUE(IsManifestValid(manifest));
  EXPECT_EQ(NO_ERROR_DETECTED, GetErrorCode());
}
