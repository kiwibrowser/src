// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/manifest_icon_downloader.h"

#include <string>
#include <vector>

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace content {

class ManifestIconDownloaderTest : public testing::Test {
 protected:
  ManifestIconDownloaderTest() = default;
  ~ManifestIconDownloaderTest() override = default;

  int FindBitmap(const int ideal_icon_size_in_px,
                 const int minimum_icon_size_in_px,
                 const std::vector<SkBitmap>& bitmaps) {
    return ManifestIconDownloader::FindClosestBitmapIndex(
        ideal_icon_size_in_px, minimum_icon_size_in_px, bitmaps);
  }

  SkBitmap CreateDummyBitmap(int width, int height) {
    SkBitmap bitmap;
    bitmap.allocN32Pixels(width, height);
    bitmap.setImmutable();
    return bitmap;
  }

  DISALLOW_COPY_AND_ASSIGN(ManifestIconDownloaderTest);
};

TEST_F(ManifestIconDownloaderTest, NoIcons) {
  ASSERT_EQ(-1, FindBitmap(0, 0, std::vector<SkBitmap>()));
}

TEST_F(ManifestIconDownloaderTest, ExactIsChosen) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(10, 10));

  ASSERT_EQ(0, FindBitmap(10, 0, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, BiggerIsChosen) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(20, 20));

  ASSERT_EQ(0, FindBitmap(10, 0, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, SmallerBelowMinimumIsIgnored) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(10, 10));

  ASSERT_EQ(-1, FindBitmap(20, 15, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, SmallerAboveMinimumIsChosen) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(15, 15));

  ASSERT_EQ(0, FindBitmap(20, 15, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, ExactIsPreferredOverBigger) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(20, 20));
  bitmaps.push_back(CreateDummyBitmap(10, 10));

  ASSERT_EQ(1, FindBitmap(10, 0, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, ExactIsPreferredOverSmaller) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(20, 20));
  bitmaps.push_back(CreateDummyBitmap(10, 10));

  ASSERT_EQ(0, FindBitmap(20, 0, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, BiggerIsPreferredOverCloserSmaller) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(20, 20));
  bitmaps.push_back(CreateDummyBitmap(10, 10));

  ASSERT_EQ(0, FindBitmap(11, 0, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, ClosestToExactIsChosen) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(25, 25));
  bitmaps.push_back(CreateDummyBitmap(20, 20));

  ASSERT_EQ(1, FindBitmap(10, 0, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, MixedReturnsBiggestClosest) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(10, 10));
  bitmaps.push_back(CreateDummyBitmap(8, 8));
  bitmaps.push_back(CreateDummyBitmap(6, 6));

  ASSERT_EQ(0, FindBitmap(9, 0, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, MixedCanReturnMiddle) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(10, 10));
  bitmaps.push_back(CreateDummyBitmap(8, 8));
  bitmaps.push_back(CreateDummyBitmap(6, 6));

  ASSERT_EQ(1, FindBitmap(7, 0, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, SquareIsPickedOverNonSquare) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(5, 5));
  bitmaps.push_back(CreateDummyBitmap(10, 15));

  ASSERT_EQ(0, FindBitmap(15, 5, bitmaps));
  ASSERT_EQ(0, FindBitmap(10, 5, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, MostSquareNonSquareIsPicked) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(25, 35));
  bitmaps.push_back(CreateDummyBitmap(10, 11));

  ASSERT_EQ(1, FindBitmap(25, 0, bitmaps));
  ASSERT_EQ(1, FindBitmap(35, 0, bitmaps));
}

TEST_F(ManifestIconDownloaderTest, NonSquareBelowMinimumIsNotPicked) {
  std::vector<SkBitmap> bitmaps;
  bitmaps.push_back(CreateDummyBitmap(10, 15));
  bitmaps.push_back(CreateDummyBitmap(15, 10));

  ASSERT_EQ(-1, FindBitmap(15, 11, bitmaps));
}

}  // namespace content
