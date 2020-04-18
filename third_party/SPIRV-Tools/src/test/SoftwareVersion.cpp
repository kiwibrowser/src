// Copyright (c) 2015-2016 Google Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

#include "UnitSPIRV.h"

#include <sstream>

#include "gmock/gmock.h"

using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::StartsWith;

namespace {

void CheckFormOfHighLevelVersion(const std::string& version) {
  std::istringstream s(version);
  char v = 'x';
  int year = -1;
  char period = 'x';
  int index = -1;
  s >> v >> year >> period >> index;
  EXPECT_THAT(v, Eq('v'));
  EXPECT_THAT(year, Ge(2016));
  EXPECT_THAT(period, Eq('.'));
  EXPECT_THAT(index, Ge(0));
  EXPECT_TRUE(s.good() || s.eof());

  std::string rest;
  s >> rest;
  EXPECT_THAT(rest, AnyOf("", "-dev"));
}

TEST(SoftwareVersion, ShortIsCorrectForm) {
  SCOPED_TRACE("short form");
  CheckFormOfHighLevelVersion(spvSoftwareVersionString());
}

TEST(SoftwareVersion, DetailedIsCorrectForm) {
  const std::string detailed_version(spvSoftwareVersionDetailsString());
  EXPECT_THAT(detailed_version, StartsWith("SPIRV-Tools v"));

  // Parse the high level version.
  const std::string from_v =
      detailed_version.substr(detailed_version.find_first_of('v'));
  const size_t first_space_after_v_or_npos = from_v.find_first_of(' ');
  SCOPED_TRACE(detailed_version);
  CheckFormOfHighLevelVersion(from_v.substr(0, first_space_after_v_or_npos));

  // We don't actually care about what comes after the version number.
}

}  // anonymous namespace
