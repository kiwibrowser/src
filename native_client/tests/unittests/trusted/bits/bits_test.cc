/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test the portability bits functions work properly for a wide range of
 * inputs (limited by test runtime).
 */

#include <limits>
#include "gtest/gtest.h"
#include "native_client/src/include/portability_bits.h"

class BitsTest : public testing::Test {
 protected:
  virtual void SetUp() {}
  virtual void TearDown() {}

  template<typename Ret, typename In>
  void TestAllInputsInRange(Ret (*impl)(In), Ret (*test)(In),
                            In start, In end) {
    EXPECT_LE(start, end);
    In i = start;
    do {
      Ret impl_result = (*impl)(i);
      Ret test_result = (*test)(i);
      EXPECT_EQ(impl_result, test_result);
    } while (i++ != end);
  }

  template<typename Ret, typename In>
  void TestAllInputs(Ret (*impl)(In), Ret (*test)(In)) {
    TestAllInputsInRange(impl, test, (In)0, std::numeric_limits<In>::max());
  }
};

namespace bits_test {

template<typename T> struct Helper { /* Specialized below. */ };
template<> struct Helper<uint8_t>  { static int Bits() { return  8; } };
template<> struct Helper<uint16_t> { static int Bits() { return 16; } };
template<> struct Helper<uint32_t> { static int Bits() { return 32; } };
template<> struct Helper<uint64_t> { static int Bits() { return 64; } };

template<typename T> INLINE int PopCount(T v) {
  int count = 0;
  while (v) {
    count += (v & 1);
    v >>= 1;
  }
  return count;
}

template<typename T> INLINE T BitReverse(T v) {
  T result = 0;
  const int bits = Helper<T>::Bits();
  for (int i = 0; i < bits / 2; ++i) {
    // Rotate LSBs to MSBs.
    T mask = (1 << ((bits - 1) - i));
    int rotation = (bits - 1) - (i << 1);
    result |= (v << rotation) & mask;
  }
  for (int i = bits / 2; i < bits; ++i) {
    // Rotate MSBs to LSBs.
    T mask = (1 << ((bits - 1) - i));
    int rotation = ((i - ((bits / 2) - 1)) << 1) - 1;
    result |= (v >> rotation) & mask;
  }
  return result;
}

template<typename T> INLINE int CountTrailingZeroes(T v) {
  int i;
  if (!v) return -1;
  for (i = 0; !(v & 1); v >>= 1, ++i) {
  }
  return i;
}

template<typename T> INLINE int CountLeadingZeroes(T v) {
  int i;
  const int last_bit = Helper<T>::Bits() - 1;
  const T last_bit_mask = (T)1 << last_bit;
  if (!v) return -1;
  for (i = 0; !(v & last_bit_mask); v <<= 1, ++i) {
  }
  return i;
}

}  // namespace bits_test


// Extra range amount, used to test a bit past interesting values.
static const int kExtra = 2050;


TEST_F(BitsTest, PopCount8) {
  TestAllInputs(&nacl::PopCount<uint8_t>,
                &bits_test::PopCount<uint8_t>);
}
TEST_F(BitsTest, PopCount16) {
  TestAllInputs(&nacl::PopCount<uint16_t>,
                &bits_test::PopCount<uint16_t>);
}
TEST_F(BitsTest, PopCount32) {
  TestAllInputsInRange(&nacl::PopCount<uint32_t>,
                       &bits_test::PopCount<uint32_t>,
                       (uint32_t)0,
                       (uint32_t)std::numeric_limits<uint16_t>::max() + kExtra);
  TestAllInputsInRange(&nacl::PopCount<uint32_t>,
                       &bits_test::PopCount<uint32_t>,
                       (uint32_t)std::numeric_limits<uint32_t>::max() -
                       std::numeric_limits<uint16_t>::max() - kExtra,
                       std::numeric_limits<uint32_t>::max());
}
TEST_F(BitsTest, PopCount64) {
  TestAllInputsInRange(&nacl::PopCount<uint64_t>,
                       &bits_test::PopCount<uint64_t>,
                       (uint64_t)0,
                       (uint64_t)std::numeric_limits<uint16_t>::max() + kExtra);
  TestAllInputsInRange(&nacl::PopCount<uint64_t>,
                       &bits_test::PopCount<uint64_t>,
                       (uint64_t)std::numeric_limits<uint32_t>::max() -
                       std::numeric_limits<uint16_t>::max() - kExtra,
                       (uint64_t)std::numeric_limits<uint32_t>::max() +
                       std::numeric_limits<uint16_t>::max() + kExtra);
  TestAllInputsInRange(&nacl::PopCount<uint64_t>,
                       &bits_test::PopCount<uint64_t>,
                       std::numeric_limits<uint64_t>::max() -
                       std::numeric_limits<uint16_t>::max() - kExtra,
                       std::numeric_limits<uint64_t>::max());
}

TEST_F(BitsTest, BitReverse32) {
  TestAllInputsInRange(&nacl::BitReverse<uint32_t>,
                       &bits_test::BitReverse<uint32_t>,
                       (uint32_t)0,
                       (uint32_t)std::numeric_limits<uint16_t>::max() + kExtra);
  TestAllInputsInRange(&nacl::BitReverse<uint32_t>,
                       &bits_test::BitReverse<uint32_t>,
                       (uint32_t)std::numeric_limits<uint32_t>::max() -
                       std::numeric_limits<uint16_t>::max() - kExtra,
                       std::numeric_limits<uint32_t>::max());
}

TEST_F(BitsTest, CountTrailingZeroes32) {
  TestAllInputsInRange(&nacl::CountTrailingZeroes<uint32_t>,
                       &bits_test::CountTrailingZeroes<uint32_t>,
                       (uint32_t)0,
                       (uint32_t)std::numeric_limits<uint16_t>::max() + kExtra);
  TestAllInputsInRange(&nacl::CountTrailingZeroes<uint32_t>,
                       &bits_test::CountTrailingZeroes<uint32_t>,
                       (uint32_t)std::numeric_limits<uint32_t>::max() -
                       std::numeric_limits<uint16_t>::max() - kExtra,
                       std::numeric_limits<uint32_t>::max());
}

TEST_F(BitsTest, CountLeadingZeroes32) {
  TestAllInputsInRange(&nacl::CountLeadingZeroes<uint32_t>,
                       &bits_test::CountLeadingZeroes<uint32_t>,
                       (uint32_t)0,
                       (uint32_t)std::numeric_limits<uint16_t>::max() + kExtra);
  TestAllInputsInRange(&nacl::CountLeadingZeroes<uint32_t>,
                       &bits_test::CountLeadingZeroes<uint32_t>,
                       (uint32_t)std::numeric_limits<uint32_t>::max() -
                       std::numeric_limits<uint16_t>::max() - kExtra,
                       std::numeric_limits<uint32_t>::max());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
