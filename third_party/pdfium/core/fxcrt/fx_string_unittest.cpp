// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>

#include "core/fxcrt/fx_string.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(fxstring, FX_atonum) {
  int i;
  EXPECT_TRUE(FX_atonum("10", &i));
  EXPECT_EQ(10, i);

  EXPECT_TRUE(FX_atonum("-10", &i));
  EXPECT_EQ(-10, i);

  EXPECT_TRUE(FX_atonum("+10", &i));
  EXPECT_EQ(10, i);

  EXPECT_TRUE(FX_atonum("-2147483648", &i));
  EXPECT_EQ(std::numeric_limits<int>::min(), i);

  EXPECT_TRUE(FX_atonum("2147483647", &i));
  EXPECT_EQ(2147483647, i);

  // Value overflows.
  EXPECT_TRUE(FX_atonum("-2147483649", &i));
  EXPECT_EQ(0, i);

  // Value overflows.
  EXPECT_TRUE(FX_atonum("+2147483648", &i));
  EXPECT_EQ(0, i);

  // Value overflows.
  EXPECT_TRUE(FX_atonum("4223423494965252", &i));
  EXPECT_EQ(0, i);

  // No explicit sign will allow the number to go negative. This is for things
  // like the encryption Permissions flag (Table 3.20 PDF 1.7 spec)
  EXPECT_TRUE(FX_atonum("4294965252", &i));
  EXPECT_EQ(-2044, i);

  EXPECT_TRUE(FX_atonum("-4294965252", &i));
  EXPECT_EQ(0, i);

  EXPECT_TRUE(FX_atonum("+4294965252", &i));
  EXPECT_EQ(0, i);

  float f;
  EXPECT_FALSE(FX_atonum("3.24", &f));
  EXPECT_FLOAT_EQ(3.24f, f);
}
