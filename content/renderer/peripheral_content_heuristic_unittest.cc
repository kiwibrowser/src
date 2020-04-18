// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/peripheral_content_heuristic.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

namespace {

const char kSameOrigin[] = "http://same.com";
const char kOtherOrigin[] = "http://other.com";

}  // namespaces

TEST(PeripheralContentHeuristic, AllowSameOrigin) {
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_ESSENTIAL_SAME_ORIGIN,
            PeripheralContentHeuristic::GetPeripheralStatus(
                std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kSameOrigin)), gfx::Size(100, 100)));
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_ESSENTIAL_SAME_ORIGIN,
            PeripheralContentHeuristic::GetPeripheralStatus(
                std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kSameOrigin)), gfx::Size(1000, 1000)));
}

TEST(PeripheralContentHeuristic, DisallowCrossOriginUnlessLarge) {
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_PERIPHERAL,
            PeripheralContentHeuristic::GetPeripheralStatus(
                std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kOtherOrigin)), gfx::Size(100, 100)));
  EXPECT_EQ(
      RenderFrame::CONTENT_STATUS_ESSENTIAL_CROSS_ORIGIN_BIG,
      PeripheralContentHeuristic::GetPeripheralStatus(
          std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
          url::Origin::Create(GURL(kOtherOrigin)), gfx::Size(1000, 1000)));
}

TEST(PeripheralContentHeuristic, TinyContent) {
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_TINY,
            PeripheralContentHeuristic::GetPeripheralStatus(
                std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kSameOrigin)), gfx::Size(1, 1)));
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_TINY,
            PeripheralContentHeuristic::GetPeripheralStatus(
                std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kOtherOrigin)), gfx::Size(1, 1)));
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_TINY,
            PeripheralContentHeuristic::GetPeripheralStatus(
                std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kOtherOrigin)), gfx::Size(5, 5)));
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_PERIPHERAL,
            PeripheralContentHeuristic::GetPeripheralStatus(
                std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kOtherOrigin)), gfx::Size(10, 10)));
}

TEST(PeripheralContentHeuristic, TemporaryOriginWhitelist) {
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_PERIPHERAL,
            PeripheralContentHeuristic::GetPeripheralStatus(
                std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kOtherOrigin)), gfx::Size(100, 100)));

  std::set<url::Origin> whitelist;
  whitelist.insert(url::Origin::Create(GURL(kOtherOrigin)));

  EXPECT_EQ(RenderFrame::CONTENT_STATUS_ESSENTIAL_CROSS_ORIGIN_WHITELISTED,
            PeripheralContentHeuristic::GetPeripheralStatus(
                whitelist, url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kOtherOrigin)), gfx::Size(100, 100)));
}

TEST(PeripheralContentHeuristic, UndefinedSize) {
  // Undefined size plugins from any origin (including same-origin and
  // whitelisted origins) are marked tiny until proven otherwise.
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_TINY,
            PeripheralContentHeuristic::GetPeripheralStatus(
                std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kSameOrigin)), gfx::Size()));

  std::set<url::Origin> whitelist;
  whitelist.insert(url::Origin::Create(GURL(kOtherOrigin)));
  EXPECT_EQ(RenderFrame::CONTENT_STATUS_TINY,
            PeripheralContentHeuristic::GetPeripheralStatus(
                whitelist, url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kOtherOrigin)), gfx::Size()));

  EXPECT_EQ(RenderFrame::CONTENT_STATUS_TINY,
            PeripheralContentHeuristic::GetPeripheralStatus(
                std::set<url::Origin>(), url::Origin::Create(GURL(kSameOrigin)),
                url::Origin::Create(GURL(kOtherOrigin)), gfx::Size()));
}

}  // namespace content
