/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdio.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/checked_cast.h"

#if NACL_WINDOWS
extern "C" {
  int TestFFS();

# ifdef _WIN64
  int TestTlsAccess();
# endif
}
#endif

using nacl::can_cast;
using nacl::saturate_cast;
using nacl::assert_cast;


#ifdef DEBUG
#define CHECK(current,expected)if((current) != (expected)) {                \
                                ++errors;                                   \
                                printf("ERROR: expected %d got %d\n",       \
                                       static_cast<int>(expected),          \
                                       static_cast<int>(current));          \
                                } else { printf("SUCCESS: expected %d"      \
                                                " got %d\n",                \
                                                static_cast<int>(expected), \
                                                static_cast<int>(current)); \
                                }
#else
#define CHECK(current,expected)if((current) != (expected)) { ++errors; }
#endif
int TestCheckedCastSaturate() {
  int errors = 0;

  uint32_t u32 = 0xffffffff;
  int32_t i32 = -0x12345678;

  uint8_t u8;
  int8_t  i8;

  CHECK(can_cast<uint8_t>(u32), false);
  u8 = saturate_cast<uint8_t>(u32);
  CHECK(u8, 255);

  CHECK(can_cast<uint8_t>(i32), false);
  u8 = saturate_cast<uint8_t>(i32);
  CHECK(u8, 0);

  i8 = saturate_cast<int8_t>(u32);
  CHECK(i8, 127);

  i8 = saturate_cast<int8_t>(i32);
  CHECK(i8, -128);

  CHECK(can_cast<uint16_t>(u32), false);
  CHECK(can_cast<int32_t>(u32), false);
  CHECK(can_cast<uint32_t>(u32), true);
  return errors;
}

int TestCheckedCastFatal() {
  int errors = 0;

  uint32_t u32 = 0xffffffff;
  int32_t i32 = -0x12345678;

  uint8_t u8;
  int8_t  i8;

  u8 = assert_cast<uint8_t>(u32);
  CHECK(u8, 255);

  u8 = assert_cast<uint8_t>(i32);
  CHECK(u8, 0);

  i8 = assert_cast<int8_t>(u32);
  CHECK(i8, 127);

  i8 = assert_cast<int8_t>(i32);
  CHECK(i8, -128);

  return errors;
}

template<int N>
class intN_t {
 public:
  static const int bits = N;
  static const int max = ((1 << (bits - 1)) - 1);
  static const int min = -max - 1;

  intN_t() : overflow_(0), storage_(0) {}

  explicit intN_t(int32_t v) {
    storage_ = v;
    if (v > max || v < min) {
      overflow_ = 1;
    } else {
      overflow_ = 0;
    }
  }

  operator int() const { return storage_; }

  bool Overflow() const { return !!overflow_; }

 private:
  int32_t overflow_  : 2;  // Mac compiler complains if this is 1 instead of 2
  int32_t storage_   : bits;
};

typedef intN_t<28> int28_t;

namespace std {
  template<> struct numeric_limits<int28_t>
  : public numeric_limits<int> {
    static const bool is_specialized = true;
    static int28_t max() {return int28_t(int28_t::max);}
    static int28_t min() {return int28_t(int28_t::min);}
//    static const int digits = 28;
  };
}  // namespace std

int TestCheckedCastUDT() {
  int32_t i32 = 0xf0000000;
  uint32_t u32 = 0xffffffff;

  int errors = 0;

  int28_t i28 = int28_t(0xffffffff);
  CHECK(i28.Overflow(), false);

  i28 = int28_t(0x80000000);
  CHECK(i28.Overflow(), true);

  CHECK(can_cast<int28_t>(i32), false);
  i28 = saturate_cast<int28_t>(i32);
  CHECK(i28.Overflow(), false);
  CHECK(i28, static_cast<int>(0xf8000000));

  i28 = saturate_cast<int28_t>(u32);
  CHECK(i28.Overflow(), false);
  CHECK(i28, static_cast<int>(0x07ffffff));


  return errors;
}

/******************************************************************************
 * main
 *****************************************************************************/
int main(int ,
         char **) {
  int errors = 0;

  errors += TestCheckedCastSaturate();
  printf("---------- After TestCheckedSaturate errors = %d\n", errors);
  errors += TestCheckedCastUDT();
  printf("---------- After TestCheckedCastUDT errors = %d\n", errors);

#if NACL_WINDOWS
  // this test ensures the validity of the assembly version of ffs()
  errors += TestFFS();
#endif

  printf("---------- Final error count = %d\n", errors);

#if NACL_WINDOWS && defined(_WIN64)
  errors += TestTlsAccess();
#endif

  return errors;
}
