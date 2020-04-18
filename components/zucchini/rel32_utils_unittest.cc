// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zucchini/rel32_utils.h"

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/optional.h"
#include "base/test/gtest_util.h"
#include "components/zucchini/address_translator.h"
#include "components/zucchini/image_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

namespace {

// A trivial AddressTranslator that applies constant shift.
class TestAddressTranslator : public AddressTranslator {
 public:
  TestAddressTranslator(offset_t image_size, rva_t rva_begin) {
    DCHECK_GE(rva_begin, 0U);
    CHECK_EQ(AddressTranslator::kSuccess,
             Initialize({{0, image_size, rva_begin, image_size}}));
  }
};

// Checks that |reader| emits and only emits |expected_refs|, in order.
void CheckReader(const std::vector<Reference>& expected_refs,
                 ReferenceReader* reader) {
  for (Reference expected_ref : expected_refs) {
    auto ref = reader->GetNext();
    EXPECT_TRUE(ref.has_value());
    EXPECT_EQ(expected_ref, ref.value());
  }
  EXPECT_EQ(base::nullopt, reader->GetNext());  // Nothing should be left.
}

}  // namespace

TEST(Rel32UtilsTest, Rel32ReaderX86) {
  constexpr offset_t kTestImageSize = 0x00100000U;
  constexpr rva_t kRvaBegin = 0x00030000U;
  TestAddressTranslator translator(kTestImageSize, kRvaBegin);

  // For simplicity, test data is not real X86 machine code. We are only
  // including rel32 targets, without the full instructions.
  std::vector<uint8_t> bytes = {
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030000: (Filler)
      0xFF, 0xFF, 0xFF, 0xFF,  // 0003000C: (Filler)
      0x04, 0x00, 0x00, 0x00,  // 00030008: 00030010
      0xFF, 0xFF, 0xFF, 0xFF,  // 0003000C: (Filler)
      0x00, 0x00, 0x00, 0x00,  // 00030010: 00030014
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030014: (Filler)
      0xF4, 0xFF, 0xFF, 0xFF,  // 00030018: 00030010
      0xE4, 0xFF, 0xFF, 0xFF,  // 0003001C: 00030004
  };
  ConstBufferView buffer(bytes.data(), bytes.size());
  // Specify rel32 locations directly, instead of parsing.
  std::vector<offset_t> rel32_locations = {0x0008U, 0x0010U, 0x0018U, 0x001CU};

  // Generate everything.
  Rel32ReaderX86 reader1(buffer, 0x0000U, 0x0020U, &rel32_locations,
                         translator);
  CheckReader({{0x0008U, 0x0010U},
               {0x0010U, 0x0014U},
               {0x0018U, 0x0010U},
               {0x001CU, 0x0004U}},
              &reader1);

  // Exclude last.
  Rel32ReaderX86 reader2(buffer, 0x0000U, 0x001CU, &rel32_locations,
                         translator);
  CheckReader({{0x0008U, 0x0010U}, {0x0010U, 0x0014U}, {0x0018U, 0x0010U}},
              &reader2);

  // Only find one.
  Rel32ReaderX86 reader3(buffer, 0x000CU, 0x0018U, &rel32_locations,
                         translator);
  CheckReader({{0x0010U, 0x0014U}}, &reader3);
}

TEST(Rel32UtilsTest, Rel32WriterX86) {
  constexpr offset_t kTestImageSize = 0x00100000U;
  constexpr rva_t kRvaBegin = 0x00030000U;
  TestAddressTranslator translator(kTestImageSize, kRvaBegin);

  std::vector<uint8_t> bytes(32, 0xFF);
  MutableBufferView buffer(bytes.data(), bytes.size());

  Rel32WriterX86 writer(buffer, translator);
  writer.PutNext({0x0008U, 0x0010U});
  EXPECT_EQ(0x00000004U, buffer.read<uint32_t>(0x08));  // 00030008: 00030010

  writer.PutNext({0x0010U, 0x0014U});
  EXPECT_EQ(0x00000000U, buffer.read<uint32_t>(0x10));  // 00030010: 00030014

  writer.PutNext({0x0018U, 0x0010U});
  EXPECT_EQ(0xFFFFFFF4U, buffer.read<uint32_t>(0x18));  // 00030018: 00030010

  writer.PutNext({0x001CU, 0x0004U});
  EXPECT_EQ(0xFFFFFFE4U, buffer.read<uint32_t>(0x1C));  // 0003001C: 00030004

  EXPECT_EQ(std::vector<uint8_t>({
                0xFF, 0xFF, 0xFF, 0xFF,  // 00030000: (Filler)
                0xFF, 0xFF, 0xFF, 0xFF,  // 00030004: (Filler)
                0x04, 0x00, 0x00, 0x00,  // 00030008: 00030010
                0xFF, 0xFF, 0xFF, 0xFF,  // 0003000C: (Filler)
                0x00, 0x00, 0x00, 0x00,  // 00030010: 00030014
                0xFF, 0xFF, 0xFF, 0xFF,  // 00030014: (Filler)
                0xF4, 0xFF, 0xFF, 0xFF,  // 00030018: 00030010
                0xE4, 0xFF, 0xFF, 0xFF,  // 0003001C: 00030004
            }),
            bytes);
}

}  // namespace zucchini
