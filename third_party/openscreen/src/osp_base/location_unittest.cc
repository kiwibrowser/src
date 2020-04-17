// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp_base/location.h"

#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
using ::testing::MatchesRegex;

TEST(LocationTest, DefaultInitializedLocationIsNullptr) {
  const Location loc;

  EXPECT_EQ(nullptr, loc.program_counter());
  EXPECT_EQ("pc:NULL", loc.ToString());
}

TEST(LocationTest, ExpresslyInitializedLocationIsValid) {
  const void* void_ptr = reinterpret_cast<void*>(0x1337);
  const Location loc(void_ptr);
  EXPECT_EQ(void_ptr, loc.program_counter());
  EXPECT_EQ("pc:0x1337", loc.ToString());
}

TEST(LocationTest, LocationFromHereIsValid) {
  const Location loc_from_here = openscreen::Location::CreateFromHere();
  const Location loc_from_here_macro = CURRENT_LOCATION;

  EXPECT_NE(nullptr, loc_from_here.program_counter());
  EXPECT_NE(nullptr, loc_from_here_macro.program_counter());

  EXPECT_THAT(loc_from_here.ToString(), MatchesRegex("pc:0x[0-9a-f]+"));
  EXPECT_THAT(loc_from_here_macro.ToString(), MatchesRegex("pc:0x[0-9a-f]+"));
}
}  // namespace openscreen
