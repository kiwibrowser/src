// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/url_utils.h"

#include "content/public/common/browser_side_navigation_policy.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

TEST(UrlUtilsTest, IsURLHandledByNetworkStack) {
  EXPECT_TRUE(IsURLHandledByNetworkStack(GURL("http://foo/bar.html")));
  EXPECT_TRUE(IsURLHandledByNetworkStack(GURL("https://foo/bar.html")));
  EXPECT_TRUE(IsURLHandledByNetworkStack(GURL("data://foo")));
  EXPECT_TRUE(IsURLHandledByNetworkStack(GURL("cid:foo@bar")));

  EXPECT_FALSE(IsURLHandledByNetworkStack(GURL("about:blank")));
  EXPECT_FALSE(IsURLHandledByNetworkStack(GURL("about:srcdoc")));
  EXPECT_FALSE(IsURLHandledByNetworkStack(GURL("javascript:foo.js")));
  EXPECT_FALSE(IsURLHandledByNetworkStack(GURL()));
}

}  // namespace content
