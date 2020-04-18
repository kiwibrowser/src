/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 * Copyright 2012, Google Inc.
 */

/*
 * A simple test for AddressSet.
 */

#include <vector>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include "gtest/gtest.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/validator_mips/address_set.h"

using nacl_mips_val::AddressSet;


class AddressTests : public ::testing::Test {
};

TEST_F(AddressTests, TestMutation) {
  uint32_t base = 0x1234;
  uint32_t size = 0x1000;
  AddressSet as(base, size);

  as.Add(0x1200);  // Becomes a no-op.
  as.Add(base + (31 * 4));  // Added.
  as.Add(0x1240);  // Added.
  as.Add(0x1230);  // No-op.
  as.Add(base + size);  // No-op.
  as.Add(0x1235);  // Added as 1234.
  as.Add(0x1238);  // Added.
  as.Add(0x2000);  // Added.
  as.Add(base + size + 100);  // No-op.
  as.Add(0x1400);  // Added.

  // Successful additions in ascending order:
  uint32_t expected[] = { 0x1234, 0x1238, 0x1240, base+(31*4), 0x1400, 0x2000 };
  for (uint32_t i = 0; i < NACL_ARRAY_SIZE(expected); i++) {
     EXPECT_TRUE(as.Contains(expected[i])) << "Set should contain "
                                           << std::hex << expected[i]
                                           << ", does not.";
  }

  uint32_t x = 0;
  for (AddressSet::Iterator it = as.Begin(); !it.Equals(as.End());
       it.Next(), ++x) {
    EXPECT_EQ(it.GetAddress(), expected[x]) << "At " << x
                                            << "\n Expected: " << std::hex
                                            << expected[x]
                                            << "\n Actual: " << std::hex
                                            << it.GetAddress();
  }
  EXPECT_EQ(x, NACL_ARRAY_SIZE(expected)) << "Expected iterator to step"
                                          << NACL_ARRAY_SIZE(expected)
                                          << " times \n" << "Actual: " << x;

  // Unsuccessful additions:
  uint32_t unexpected[] = { 0x1200, 0x1230, base+size, base+size+100 };
  for (uint32_t i = 0; i < NACL_ARRAY_SIZE(unexpected); i++) {
    EXPECT_FALSE(as.Contains(unexpected[i])) << "Set should not contain "
                                           << std::hex << unexpected[i];
  }
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
