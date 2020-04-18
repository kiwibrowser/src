// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/media_route.h"

#include "chrome/common/media_router/media_sink.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace {
constexpr char kRouteId1[] =
    "urn:x-org.chromium:media:route:1/cast-sink1/http://foo.com";
constexpr char kRouteId2[] =
    "urn:x-org.chromium:media:route:2/cast-sink2/http://foo.com";
constexpr char kPresentationUrl[] = "http://www.example.com/presentation.html";
}  // namespace

namespace media_router {

// Tests the == operator to ensure that only route ID equality is being checked.
TEST(MediaRouteTest, Equals) {
  const MediaSource& media_source =
      MediaSourceForPresentationUrl(GURL(kPresentationUrl));
  MediaRoute route1(kRouteId1, media_source, "sinkId", "Description", false,
                    false);

  // Same as route1 with different sink ID.
  MediaRoute route2(kRouteId1, media_source, "differentSinkId", "Description",
                    false, false);
  EXPECT_TRUE(route1.Equals(route2));

  // Same as route1 with different description.
  MediaRoute route3(kRouteId1, media_source, "sinkId", "differentDescription",
                    false, false);
  EXPECT_TRUE(route1.Equals(route3));

  // Same as route1 with different is_local.
  MediaRoute route4(kRouteId1, media_source, "sinkId", "Description", true,
                    false);
  EXPECT_TRUE(route1.Equals(route4));

  // The ID is different from route1's.
  MediaRoute route5(kRouteId2, media_source, "sinkId", "Description", false,
                    false);
  EXPECT_FALSE(route1.Equals(route5));

  // Same as route1 with different incognito.
  MediaRoute route6(kRouteId1, media_source, "sinkId", "Description", true,
                    false);
  route6.set_incognito(true);
  EXPECT_TRUE(route1.Equals(route6));
}

}  // namespace media_router
