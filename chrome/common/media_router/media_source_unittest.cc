// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/media_source.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

// Test that the object's getters match the constructor parameters.
TEST(MediaSourceTest, Constructor) {
  MediaSource source1("urn:x-com.google.cast:application:DEADBEEF");
  EXPECT_EQ("urn:x-com.google.cast:application:DEADBEEF", source1.id());
  EXPECT_EQ(GURL(""), source1.url());
}

TEST(MediaSourceTest, ConstructorWithGURL) {
  GURL test_url = GURL("http://google.com");
  MediaSource source1(test_url);
  EXPECT_EQ(test_url.spec(), source1.id());
  EXPECT_EQ(test_url, source1.url());
}

TEST(MediaSourceTest, ConstructorWithURLString) {
  GURL test_url = GURL("http://google.com");
  MediaSource source1(test_url.spec());
  EXPECT_EQ(test_url.spec(), source1.id());
  EXPECT_EQ(test_url, source1.url());
}

}  // namespace media_router
