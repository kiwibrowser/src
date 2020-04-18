// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/media_source_helper.h"

#include "chrome/common/media_router/media_source.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace media_router {

constexpr char kPresentationUrl[] = "https://www.example.com/presentation.html";

TEST(MediaSourcesTest, IsMirroringMediaSource) {
  EXPECT_TRUE(IsTabMirroringMediaSource(MediaSourceForTab(123)));
  EXPECT_TRUE(IsDesktopMirroringMediaSource(MediaSourceForDesktop()));
  EXPECT_TRUE(IsMirroringMediaSource(MediaSourceForTab(123)));
  EXPECT_TRUE(IsMirroringMediaSource(MediaSourceForDesktop()));
  EXPECT_FALSE(IsMirroringMediaSource(
      MediaSourceForPresentationUrl(GURL(kPresentationUrl))));
}

TEST(MediaSourcesTest, CreateMediaSource) {
  EXPECT_EQ("urn:x-org.chromium.media:source:tab:123",
            MediaSourceForTab(123).id());
  EXPECT_EQ("urn:x-org.chromium.media:source:desktop",
            MediaSourceForDesktop().id());
  EXPECT_EQ(kPresentationUrl,
            MediaSourceForPresentationUrl(GURL(kPresentationUrl)).id());
}

TEST(MediaSourcesTest, IsValidMediaSource) {
  EXPECT_TRUE(IsValidMediaSource(MediaSourceForTab(123)));
  EXPECT_TRUE(IsValidMediaSource(MediaSourceForDesktop()));
  EXPECT_TRUE(IsValidMediaSource(
      MediaSourceForPresentationUrl(GURL(kPresentationUrl))));

  // Disallowed scheme
  EXPECT_FALSE(IsValidMediaSource(
      MediaSourceForPresentationUrl(GURL("file:///some/local/path"))));
  // Not a URL
  EXPECT_FALSE(IsValidMediaSource(
      MediaSourceForPresentationUrl(GURL("totally not a url"))));
}

TEST(MediaSourcesTest, IsValidPresentationUrl) {
  EXPECT_FALSE(IsValidPresentationUrl(GURL()));
  EXPECT_FALSE(IsValidPresentationUrl(GURL("unsupported-scheme://foo")));

  EXPECT_TRUE(IsValidPresentationUrl(GURL("https://google.com")));
  EXPECT_TRUE(IsValidPresentationUrl(GURL("cast://foo")));
  EXPECT_TRUE(IsValidPresentationUrl(GURL("cast:foo")));
}

TEST(MediaSourcesTest, IsCastPresentationUrl) {
  EXPECT_TRUE(IsCastPresentationUrl(MediaSource(GURL("cast:233637DE"))));
  EXPECT_TRUE(IsCastPresentationUrl(
      MediaSource(GURL("https://google.com/cast#__castAppId__=233637DE"))));
  // false scheme
  EXPECT_FALSE(IsCastPresentationUrl(
      MediaSource(GURL("http://google.com/cast#__castAppId__=233637DE"))));
  // false domain
  EXPECT_FALSE(IsCastPresentationUrl(
      MediaSource(GURL("https://google2.com/cast#__castAppId__=233637DE"))));
  // empty path
  EXPECT_FALSE(
      IsCastPresentationUrl(MediaSource(GURL("https://www.google.com"))));
  // false path
  EXPECT_FALSE(
      IsCastPresentationUrl(MediaSource(GURL("https://www.google.com/path"))));

  EXPECT_FALSE(IsCastPresentationUrl(MediaSource(GURL(""))));
}

TEST(MediaSourcesTest, IsDialMediaSource) {
  EXPECT_TRUE(IsDialMediaSource(
      MediaSource("cast-dial:YouTube?dialPostData=postData&clientId=1234")));
  // false scheme
  EXPECT_FALSE(IsDialMediaSource(
      MediaSource("https://google.com/cast#__castAppId__=233637DE")));
}

TEST(MediaSourcesTest, AppNameFromDialMediaSource) {
  MediaSource media_source(
      "cast-dial:YouTube?dialPostData=postData&clientId=1234");
  EXPECT_EQ("YouTube", AppNameFromDialMediaSource(media_source));

  media_source = MediaSource("dial:YouTube");
  EXPECT_TRUE(AppNameFromDialMediaSource(media_source).empty());

  media_source = MediaSource("https://google.com/cast#__castAppId__=233637DE");
  EXPECT_TRUE(AppNameFromDialMediaSource(media_source).empty());
}

}  // namespace media_router
