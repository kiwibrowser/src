// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/manifest_icon_selector.h"

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

using IconPurpose = blink::Manifest::Icon::IconPurpose;

namespace {

const int kIdealIconSize = 144;
const int kMinimumIconSize = 0;

static blink::Manifest::Icon CreateIcon(const std::string& url,
                                        const std::string& type,
                                        const std::vector<gfx::Size> sizes,
                                        IconPurpose purpose) {
  blink::Manifest::Icon icon;
  icon.src = GURL(url);
  icon.type = base::UTF8ToUTF16(type);
  icon.sizes = sizes;
  icon.purpose.push_back(purpose);

  return icon;
}

}  // anonymous namespace

TEST(ManifestIconSelector, NoIcons) {
  // No icons should return the empty URL.
  std::vector<blink::Manifest::Icon> icons;
  GURL url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_TRUE(url.is_empty());
}

TEST(ManifestIconSelector, NoSizes) {
  // Icon with no sizes are ignored.
  std::vector<blink::Manifest::Icon> icons;
  icons.push_back(CreateIcon("http://foo.com/icon.png", "",
                             std::vector<gfx::Size>(), IconPurpose::ANY));

  GURL url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_TRUE(url.is_empty());
}

TEST(ManifestIconSelector, MIMETypeFiltering) {
  // Icons with type specified to a MIME type that isn't a valid image MIME type
  // are ignored.
  std::vector<gfx::Size> sizes;
  sizes.push_back(gfx::Size(1024, 1024));

  std::vector<blink::Manifest::Icon> icons;
  icons.push_back(CreateIcon("http://foo.com/icon.png", "image/foo_bar", sizes,
                             IconPurpose::ANY));
  icons.push_back(
      CreateIcon("http://foo.com/icon.png", "image/", sizes, IconPurpose::ANY));
  icons.push_back(
      CreateIcon("http://foo.com/icon.png", "image/", sizes, IconPurpose::ANY));
  icons.push_back(CreateIcon("http://foo.com/icon.png", "video/mp4", sizes,
                             IconPurpose::ANY));

  GURL url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_TRUE(url.is_empty());

  icons.clear();
  icons.push_back(CreateIcon("http://foo.com/icon.png", "image/png", sizes,
                             IconPurpose::ANY));
  url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon.png", url.spec());

  icons.clear();
  icons.push_back(CreateIcon("http://foo.com/icon.png", "image/gif", sizes,
                             IconPurpose::ANY));
  url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon.png", url.spec());

  icons.clear();
  icons.push_back(CreateIcon("http://foo.com/icon.png", "image/jpeg", sizes,
                             IconPurpose::ANY));
  url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon.png", url.spec());
}

TEST(ManifestIconSelector, PurposeFiltering) {
  // Icons with purpose specified to non-matching purpose are ignored.
  std::vector<gfx::Size> sizes_48;
  sizes_48.push_back(gfx::Size(48, 48));

  std::vector<gfx::Size> sizes_96;
  sizes_96.push_back(gfx::Size(96, 96));

  std::vector<gfx::Size> sizes_144;
  sizes_144.push_back(gfx::Size(144, 144));

  std::vector<blink::Manifest::Icon> icons;
  icons.push_back(CreateIcon("http://foo.com/icon_48.png", "", sizes_48,
                             IconPurpose::BADGE));
  icons.push_back(
      CreateIcon("http://foo.com/icon_96.png", "", sizes_96, IconPurpose::ANY));
  icons.push_back(CreateIcon("http://foo.com/icon_144.png", "", sizes_144,
                             IconPurpose::ANY));

  GURL url = ManifestIconSelector::FindBestMatchingIcon(
      icons, 48, kMinimumIconSize, IconPurpose::BADGE);
  EXPECT_EQ("http://foo.com/icon_48.png", url.spec());

  url = ManifestIconSelector::FindBestMatchingIcon(icons, 48, kMinimumIconSize,
                                                   IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_96.png", url.spec());

  url = ManifestIconSelector::FindBestMatchingIcon(icons, 96, kMinimumIconSize,
                                                   IconPurpose::BADGE);
  EXPECT_EQ("http://foo.com/icon_48.png", url.spec());

  url = ManifestIconSelector::FindBestMatchingIcon(icons, 96, 96,
                                                   IconPurpose::BADGE);
  EXPECT_TRUE(url.is_empty());

  url = ManifestIconSelector::FindBestMatchingIcon(icons, 144, kMinimumIconSize,
                                                   IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_144.png", url.spec());
}

TEST(ManifestIconSelector, IdealSizeIsUsedFirst) {
  // Each icon is marked with sizes that match the ideal icon size.
  std::vector<gfx::Size> sizes_48;
  sizes_48.push_back(gfx::Size(48, 48));

  std::vector<gfx::Size> sizes_96;
  sizes_96.push_back(gfx::Size(96, 96));

  std::vector<gfx::Size> sizes_144;
  sizes_144.push_back(gfx::Size(144, 144));

  std::vector<blink::Manifest::Icon> icons;
  icons.push_back(
      CreateIcon("http://foo.com/icon_48.png", "", sizes_48, IconPurpose::ANY));
  icons.push_back(
      CreateIcon("http://foo.com/icon_96.png", "", sizes_96, IconPurpose::ANY));
  icons.push_back(CreateIcon("http://foo.com/icon_144.png", "", sizes_144,
                             IconPurpose::ANY));

  GURL url = ManifestIconSelector::FindBestMatchingIcon(
      icons, 48, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_48.png", url.spec());

  url = ManifestIconSelector::FindBestMatchingIcon(icons, 96, kMinimumIconSize,
                                                   IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_96.png", url.spec());

  url = ManifestIconSelector::FindBestMatchingIcon(icons, 144, kMinimumIconSize,
                                                   IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_144.png", url.spec());
}

TEST(ManifestIconSelector, FirstIconWithIdealSizeIsUsedFirst) {
  // This test has three icons. The first icon is going to be used because it
  // contains the ideal size.
  std::vector<gfx::Size> sizes_1;
  sizes_1.push_back(gfx::Size(kIdealIconSize, kIdealIconSize));
  sizes_1.push_back(gfx::Size(kIdealIconSize * 2, kIdealIconSize * 2));
  sizes_1.push_back(gfx::Size(kIdealIconSize * 3, kIdealIconSize * 3));

  std::vector<gfx::Size> sizes_2;
  sizes_2.push_back(gfx::Size(1024, 1024));

  std::vector<gfx::Size> sizes_3;
  sizes_3.push_back(gfx::Size(1024, 1024));

  std::vector<blink::Manifest::Icon> icons;
  icons.push_back(
      CreateIcon("http://foo.com/icon_x1.png", "", sizes_1, IconPurpose::ANY));
  icons.push_back(
      CreateIcon("http://foo.com/icon_x2.png", "", sizes_2, IconPurpose::ANY));
  icons.push_back(
      CreateIcon("http://foo.com/icon_x3.png", "", sizes_3, IconPurpose::ANY));

  GURL url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_x1.png", url.spec());

  url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kIdealIconSize * 2, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_x1.png", url.spec());

  url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kIdealIconSize * 3, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_x1.png", url.spec());
}

TEST(ManifestIconSelector, FallbackToSmallestLargerIcon) {
  // If there is no perfect icon, the smallest larger icon will be chosen.
  std::vector<gfx::Size> sizes_1;
  sizes_1.push_back(gfx::Size(90, 90));

  std::vector<gfx::Size> sizes_2;
  sizes_2.push_back(gfx::Size(128, 128));

  std::vector<gfx::Size> sizes_3;
  sizes_3.push_back(gfx::Size(192, 192));

  std::vector<blink::Manifest::Icon> icons;
  icons.push_back(
      CreateIcon("http://foo.com/icon_x1.png", "", sizes_1, IconPurpose::ANY));
  icons.push_back(
      CreateIcon("http://foo.com/icon_x2.png", "", sizes_2, IconPurpose::ANY));
  icons.push_back(
      CreateIcon("http://foo.com/icon_x3.png", "", sizes_3, IconPurpose::ANY));

  GURL url = ManifestIconSelector::FindBestMatchingIcon(
      icons, 48, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_x1.png", url.spec());

  url = ManifestIconSelector::FindBestMatchingIcon(icons, 96, kMinimumIconSize,
                                                   IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_x2.png", url.spec());

  url = ManifestIconSelector::FindBestMatchingIcon(icons, 144, kMinimumIconSize,
                                                   IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_x3.png", url.spec());
}

TEST(ManifestIconSelector, FallbackToLargestIconLargerThanMinimum) {
  // When an icon of the correct size has not been found, we fall back to the
  // closest non-matching sizes. Make sure that the minimum passed is enforced.
  std::vector<gfx::Size> sizes_1_2;
  std::vector<gfx::Size> sizes_3;

  sizes_1_2.push_back(gfx::Size(47, 47));
  sizes_3.push_back(gfx::Size(95, 95));

  std::vector<blink::Manifest::Icon> icons;
  icons.push_back(CreateIcon("http://foo.com/icon_x1.png", "", sizes_1_2,
                             IconPurpose::ANY));
  icons.push_back(CreateIcon("http://foo.com/icon_x2.png", "", sizes_1_2,
                             IconPurpose::ANY));
  icons.push_back(
      CreateIcon("http://foo.com/icon_x3.png", "", sizes_3, IconPurpose::ANY));

  // Icon 3 should match.
  GURL url = ManifestIconSelector::FindBestMatchingIcon(icons, 1024, 48,
                                                        IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_x3.png", url.spec());

  // Nothing matches here as the minimum is 96.
  url = ManifestIconSelector::FindBestMatchingIcon(icons, 1024, 96,
                                                   IconPurpose::ANY);
  EXPECT_TRUE(url.is_empty());
}

TEST(ManifestIconSelector, IdealVeryCloseToMinimumMatches) {
  std::vector<gfx::Size> sizes;
  sizes.push_back(gfx::Size(2, 2));

  std::vector<blink::Manifest::Icon> icons;
  icons.push_back(
      CreateIcon("http://foo.com/icon_x1.png", "", sizes, IconPurpose::ANY));

  GURL url =
      ManifestIconSelector::FindBestMatchingIcon(icons, 2, 1, IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_x1.png", url.spec());
}

TEST(ManifestIconSelector, SizeVeryCloseToMinimumMatches) {
  std::vector<gfx::Size> sizes;
  sizes.push_back(gfx::Size(2, 2));

  std::vector<blink::Manifest::Icon> icons;
  icons.push_back(
      CreateIcon("http://foo.com/icon_x1.png", "", sizes, IconPurpose::ANY));

  GURL url = ManifestIconSelector::FindBestMatchingIcon(icons, 200, 1,
                                                        IconPurpose::ANY);
  EXPECT_EQ("http://foo.com/icon_x1.png", url.spec());
}

TEST(ManifestIconSelector, NotSquareIconsAreIgnored) {
  std::vector<gfx::Size> sizes;
  sizes.push_back(gfx::Size(1024, 1023));

  std::vector<blink::Manifest::Icon> icons;
  icons.push_back(
      CreateIcon("http://foo.com/icon.png", "", sizes, IconPurpose::ANY));

  GURL url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
  EXPECT_TRUE(url.is_empty());
}

TEST(ManifestIconSelector, ClosestIconToIdeal) {
  // Ensure ManifestIconSelector::FindBestMatchingIcon selects the closest icon
  // to the ideal size when presented with a number of options.
  int very_small = kIdealIconSize / 4;
  int small_size = kIdealIconSize / 2;
  int bit_small = kIdealIconSize - 1;
  int bit_big = kIdealIconSize + 1;
  int big = kIdealIconSize * 2;
  int very_big = kIdealIconSize * 4;

  // (very_small, bit_small) => bit_small
  {
    std::vector<gfx::Size> sizes_1;
    sizes_1.push_back(gfx::Size(very_small, very_small));

    std::vector<gfx::Size> sizes_2;
    sizes_2.push_back(gfx::Size(bit_small, bit_small));

    std::vector<blink::Manifest::Icon> icons;
    icons.push_back(CreateIcon("http://foo.com/icon_no.png", "", sizes_1,
                               IconPurpose::ANY));
    icons.push_back(
        CreateIcon("http://foo.com/icon.png", "", sizes_2, IconPurpose::ANY));

    GURL url = ManifestIconSelector::FindBestMatchingIcon(
        icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
    EXPECT_EQ("http://foo.com/icon.png", url.spec());
  }

  // (very_small, bit_small, small_size) => bit_small
  {
    std::vector<gfx::Size> sizes_1;
    sizes_1.push_back(gfx::Size(very_small, very_small));

    std::vector<gfx::Size> sizes_2;
    sizes_2.push_back(gfx::Size(bit_small, bit_small));

    std::vector<gfx::Size> sizes_3;
    sizes_3.push_back(gfx::Size(small_size, small_size));

    std::vector<blink::Manifest::Icon> icons;
    icons.push_back(CreateIcon("http://foo.com/icon_no_1.png", "", sizes_1,
                               IconPurpose::ANY));
    icons.push_back(
        CreateIcon("http://foo.com/icon.png", "", sizes_2, IconPurpose::ANY));
    icons.push_back(CreateIcon("http://foo.com/icon_no_2.png", "", sizes_3,
                               IconPurpose::ANY));

    GURL url = ManifestIconSelector::FindBestMatchingIcon(
        icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
    EXPECT_EQ("http://foo.com/icon.png", url.spec());
  }

  // (very_big, big) => big
  {
    std::vector<gfx::Size> sizes_1;
    sizes_1.push_back(gfx::Size(very_big, very_big));

    std::vector<gfx::Size> sizes_2;
    sizes_2.push_back(gfx::Size(big, big));

    std::vector<blink::Manifest::Icon> icons;
    icons.push_back(CreateIcon("http://foo.com/icon_no.png", "", sizes_1,
                               IconPurpose::ANY));
    icons.push_back(
        CreateIcon("http://foo.com/icon.png", "", sizes_2, IconPurpose::ANY));

    GURL url = ManifestIconSelector::FindBestMatchingIcon(
        icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
    EXPECT_EQ("http://foo.com/icon.png", url.spec());
  }

  // (very_big, big, bit_big) => bit_big
  {
    std::vector<gfx::Size> sizes_1;
    sizes_1.push_back(gfx::Size(very_big, very_big));

    std::vector<gfx::Size> sizes_2;
    sizes_2.push_back(gfx::Size(big, big));

    std::vector<gfx::Size> sizes_3;
    sizes_3.push_back(gfx::Size(bit_big, bit_big));

    std::vector<blink::Manifest::Icon> icons;
    icons.push_back(CreateIcon("http://foo.com/icon_no.png", "", sizes_1,
                               IconPurpose::ANY));
    icons.push_back(CreateIcon("http://foo.com/icon_no.png", "", sizes_2,
                               IconPurpose::ANY));
    icons.push_back(
        CreateIcon("http://foo.com/icon.png", "", sizes_3, IconPurpose::ANY));

    GURL url = ManifestIconSelector::FindBestMatchingIcon(
        icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
    EXPECT_EQ("http://foo.com/icon.png", url.spec());
  }

  // (bit_small, very_big) => very_big
  {
    std::vector<gfx::Size> sizes_1;
    sizes_1.push_back(gfx::Size(bit_small, bit_small));

    std::vector<gfx::Size> sizes_2;
    sizes_2.push_back(gfx::Size(very_big, very_big));

    std::vector<blink::Manifest::Icon> icons;
    icons.push_back(CreateIcon("http://foo.com/icon_no.png", "", sizes_1,
                               IconPurpose::ANY));
    icons.push_back(
        CreateIcon("http://foo.com/icon.png", "", sizes_2, IconPurpose::ANY));

    GURL url = ManifestIconSelector::FindBestMatchingIcon(
        icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
    EXPECT_EQ("http://foo.com/icon.png", url.spec());
  }

  // (bit_small, bit_big) => bit_big
  {
    std::vector<gfx::Size> sizes_1;
    sizes_1.push_back(gfx::Size(bit_small, bit_small));

    std::vector<gfx::Size> sizes_2;
    sizes_2.push_back(gfx::Size(bit_big, bit_big));

    std::vector<blink::Manifest::Icon> icons;
    icons.push_back(CreateIcon("http://foo.com/icon_no.png", "", sizes_1,
                               IconPurpose::ANY));
    icons.push_back(
        CreateIcon("http://foo.com/icon.png", "", sizes_2, IconPurpose::ANY));

    GURL url = ManifestIconSelector::FindBestMatchingIcon(
        icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
    EXPECT_EQ("http://foo.com/icon.png", url.spec());
  }
}

TEST(ManifestIconSelector, UseAnyIfNoIdealSize) {
  // 'any' (ie. gfx::Size(0,0)) should be used if there is no icon of a
  // ideal size.

  // Icon with 'any' and icon with ideal size => ideal size is chosen.
  {
    std::vector<gfx::Size> sizes_1;
    sizes_1.push_back(gfx::Size(kIdealIconSize, kIdealIconSize));
    std::vector<gfx::Size> sizes_2;
    sizes_2.push_back(gfx::Size(0, 0));

    std::vector<blink::Manifest::Icon> icons;
    icons.push_back(
        CreateIcon("http://foo.com/icon.png", "", sizes_1, IconPurpose::ANY));
    icons.push_back(CreateIcon("http://foo.com/icon_no.png", "", sizes_2,
                               IconPurpose::ANY));

    GURL url = ManifestIconSelector::FindBestMatchingIcon(
        icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
    EXPECT_EQ("http://foo.com/icon.png", url.spec());
  }

  // Icon with 'any' and icon larger than ideal size => any is chosen.
  {
    std::vector<gfx::Size> sizes_1;
    sizes_1.push_back(gfx::Size(kIdealIconSize + 1, kIdealIconSize + 1));
    std::vector<gfx::Size> sizes_2;
    sizes_2.push_back(gfx::Size(0, 0));

    std::vector<blink::Manifest::Icon> icons;
    icons.push_back(CreateIcon("http://foo.com/icon_no.png", "", sizes_1,
                               IconPurpose::ANY));
    icons.push_back(
        CreateIcon("http://foo.com/icon.png", "", sizes_2, IconPurpose::ANY));

    GURL url = ManifestIconSelector::FindBestMatchingIcon(
        icons, kIdealIconSize, kMinimumIconSize, IconPurpose::ANY);
    EXPECT_EQ("http://foo.com/icon.png", url.spec());
  }

  // Multiple icons with 'any' => the last one is chosen.
  {
    std::vector<gfx::Size> sizes;
    sizes.push_back(gfx::Size(0, 0));

    std::vector<blink::Manifest::Icon> icons;
    icons.push_back(
        CreateIcon("http://foo.com/icon_no1.png", "", sizes, IconPurpose::ANY));
    icons.push_back(
        CreateIcon("http://foo.com/icon_no2.png", "", sizes, IconPurpose::ANY));
    icons.push_back(
        CreateIcon("http://foo.com/icon.png", "", sizes, IconPurpose::ANY));

    GURL url = ManifestIconSelector::FindBestMatchingIcon(
        icons, kIdealIconSize * 3, kMinimumIconSize, IconPurpose::ANY);
    EXPECT_EQ("http://foo.com/icon.png", url.spec());
  }
}

}  // namespace content
