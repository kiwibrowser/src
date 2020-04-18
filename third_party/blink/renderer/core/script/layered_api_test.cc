// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/script/layered_api.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace layered_api {

namespace {

TEST(LayeredAPITest, ResolveFetchingURL) {
  KURL base_url("https://example.com/base/path/");

  EXPECT_EQ(ResolveFetchingURL(KURL("https://example.com/"), base_url),
            KURL("https://example.com/"));

  EXPECT_EQ(ResolveFetchingURL(KURL("std:blank"), base_url), KURL("std:blank"));
  EXPECT_EQ(ResolveFetchingURL(KURL("std:blank|https://fallback.example.com/"),
                               base_url),
            KURL("std:blank"));
  EXPECT_EQ(
      ResolveFetchingURL(KURL("std:blank|https://:invalid-url"), base_url),
      KURL("std:blank"));

  EXPECT_EQ(ResolveFetchingURL(KURL("std:none"), base_url), NullURL());
  EXPECT_EQ(ResolveFetchingURL(KURL("std:none|https://fallback.example.com/"),
                               base_url),
            KURL("https://fallback.example.com/"));
  EXPECT_FALSE(
      ResolveFetchingURL(KURL("std:none|https://:invalid-url"), base_url)
          .IsValid());

  EXPECT_EQ(ResolveFetchingURL(KURL("std:none|fallback.js"), base_url),
            KURL("https://example.com/base/path/fallback.js"));
  EXPECT_EQ(ResolveFetchingURL(KURL("std:none|./fallback.js"), base_url),
            KURL("https://example.com/base/path/fallback.js"));
  EXPECT_EQ(ResolveFetchingURL(KURL("std:none|/fallback.js"), base_url),
            KURL("https://example.com/fallback.js"));
}

TEST(LayeredAPITest, GetInternalURL) {
  EXPECT_EQ(GetInternalURL(KURL("https://example.com/")), NullURL());

  EXPECT_EQ(GetInternalURL(KURL("std:blank")),
            KURL("std-internal://blank/index.js"));

  EXPECT_EQ(GetInternalURL(KURL("std-internal://blank/index.js")),
            KURL("std-internal://blank/index.js"));
  EXPECT_EQ(GetInternalURL(KURL("std-internal://blank/foo/bar.js")),
            KURL("std-internal://blank/foo/bar.js"));
}

TEST(LayeredAPITest, InternalURLRelativeResolution) {
  EXPECT_EQ(KURL(KURL("std-internal://blank/index.js"), "./sub.js"),
            KURL("std-internal://blank/sub.js"));
  EXPECT_EQ(KURL(KURL("std-internal://blank/index.js"), "/sub.js"),
            KURL("std-internal://blank/sub.js"));
  EXPECT_EQ(KURL(KURL("std-internal://blank/index.js"), "./foo/bar.js"),
            KURL("std-internal://blank/foo/bar.js"));
  EXPECT_EQ(KURL(KURL("std-internal://blank/foo/bar.js"), "../baz.js"),
            KURL("std-internal://blank/baz.js"));
}

TEST(LayeredAPITest, GetSourceText) {
  EXPECT_EQ(GetSourceText(KURL("std-internal://blank/index.js")), String(""));

  EXPECT_EQ(GetSourceText(KURL("std-internal://blank/not-found.js")), String());
  EXPECT_EQ(GetSourceText(KURL("std-internal://none/index.js")), String());

  EXPECT_EQ(GetSourceText(KURL("https://example.com/")), String());
}

}  // namespace

}  // namespace layered_api

}  // namespace blink
