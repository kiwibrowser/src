// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/system/version_loader.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

typedef testing::Test VersionLoaderTest;

static const char kTest10[] = "vendor            | FOO";
static const char kTest11[] = "firmware          | FOO";
static const char kTest12[] = "firmware          | FOO";
static const char kTest13[] = "version           | 0.2.3.3";
static const char kTest14[] = "version        | 0.2.3.3";
static const char kTest15[] = "version             0.2.3.3";

TEST_F(VersionLoaderTest, ParseFirmware) {
  EXPECT_EQ("", version_loader::ParseFirmware(kTest10));
  EXPECT_EQ("", version_loader::ParseFirmware(kTest11));
  EXPECT_EQ("", version_loader::ParseFirmware(kTest12));
  EXPECT_EQ("0.2.3.3", version_loader::ParseFirmware(kTest13));
  EXPECT_EQ("0.2.3.3", version_loader::ParseFirmware(kTest14));
  EXPECT_EQ("0.2.3.3", version_loader::ParseFirmware(kTest15));
}

}  // namespace chromeos
