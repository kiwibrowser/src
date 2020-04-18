// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/origin_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

TEST(URLSchemesTest, IsOriginSecure) {
  EXPECT_TRUE(IsOriginSecure(GURL("file:///test/fun.html")));
  EXPECT_TRUE(IsOriginSecure(GURL("file:///test/")));

  EXPECT_TRUE(IsOriginSecure(GURL("https://example.com/fun.html")));
  EXPECT_FALSE(IsOriginSecure(GURL("http://example.com/fun.html")));

  EXPECT_TRUE(IsOriginSecure(GURL("wss://example.com/fun.html")));
  EXPECT_FALSE(IsOriginSecure(GURL("ws://example.com/fun.html")));

  EXPECT_TRUE(IsOriginSecure(GURL("http://localhost/fun.html")));
  EXPECT_TRUE(IsOriginSecure(GURL("http://pumpkin.localhost/fun.html")));
  EXPECT_TRUE(
      IsOriginSecure(GURL("http://crumpet.pumpkin.localhost/fun.html")));
  EXPECT_TRUE(IsOriginSecure(GURL("http://pumpkin.localhost:8080/fun.html")));
  EXPECT_TRUE(
      IsOriginSecure(GURL("http://crumpet.pumpkin.localhost:3000/fun.html")));
  EXPECT_FALSE(IsOriginSecure(GURL("http://localhost.com/fun.html")));
  EXPECT_TRUE(IsOriginSecure(GURL("https://localhost.com/fun.html")));

  EXPECT_TRUE(IsOriginSecure(GURL("http://127.0.0.1/fun.html")));
  EXPECT_TRUE(IsOriginSecure(GURL("ftp://127.0.0.1/fun.html")));
  EXPECT_TRUE(IsOriginSecure(GURL("http://127.3.0.1/fun.html")));
  EXPECT_FALSE(IsOriginSecure(GURL("http://127.example.com/fun.html")));
  EXPECT_TRUE(IsOriginSecure(GURL("https://127.example.com/fun.html")));

  EXPECT_TRUE(IsOriginSecure(GURL("http://[::1]/fun.html")));
  EXPECT_FALSE(IsOriginSecure(GURL("http://[::2]/fun.html")));
  EXPECT_FALSE(IsOriginSecure(GURL("http://[::1].example.com/fun.html")));

  EXPECT_FALSE(
      IsOriginSecure(GURL("filesystem:http://www.example.com/temporary/")));
  EXPECT_FALSE(
      IsOriginSecure(GURL("filesystem:ftp://www.example.com/temporary/")));
  EXPECT_TRUE(IsOriginSecure(GURL("filesystem:ftp://127.0.0.1/temporary/")));
  EXPECT_TRUE(
      IsOriginSecure(GURL("filesystem:https://www.example.com/temporary/")));
}

}  // namespace content
