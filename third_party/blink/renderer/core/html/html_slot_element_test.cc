// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/html_slot_element.h"

#include <array>

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {
constexpr int kTableSize = 16;
using Seq = std::vector<char>;
using Backtrack = std::pair<size_t, size_t>;
}

class HTMLSlotElementTest : public testing::Test {
 protected:
  HTMLSlotElementTest() = default;
  Seq LongestCommonSubsequence(const Seq& seq1, const Seq& seq2);
  std::array<std::array<size_t, kTableSize>, kTableSize> lcs_table_;
  std::array<std::array<Backtrack, kTableSize>, kTableSize> backtrack_table_;
};

std::vector<char> HTMLSlotElementTest::LongestCommonSubsequence(
    const Seq& seq1,
    const Seq& seq2) {
  HTMLSlotElement::FillLongestCommonSubsequenceDynamicProgrammingTable(
      seq1, seq2, lcs_table_, backtrack_table_);
  Seq lcs;
  size_t r = seq1.size();
  size_t c = seq2.size();
  while (r > 0 && c > 0) {
    Backtrack backtrack = backtrack_table_[r][c];
    if (backtrack == std::make_pair(r - 1, c - 1)) {
      DCHECK_EQ(seq1[r - 1], seq2[c - 1]);
      lcs.push_back(seq1[r - 1]);
    }
    std::tie(r, c) = backtrack;
  }
  std::reverse(lcs.begin(), lcs.end());
  EXPECT_EQ(lcs_table_[seq1.size()][seq2.size()], lcs.size());
  return lcs;
}

TEST_F(HTMLSlotElementTest, LongestCommonSubsequence) {
  const Seq kEmpty;
  {
    Seq seq1{};
    Seq seq2{};
    EXPECT_EQ(kEmpty, LongestCommonSubsequence(seq1, seq2));
  }
  {
    Seq seq1{'a'};
    Seq seq2{};
    EXPECT_EQ(kEmpty, LongestCommonSubsequence(seq1, seq2));
  }
  {
    Seq seq1{};
    Seq seq2{'a'};
    EXPECT_EQ(kEmpty, LongestCommonSubsequence(seq1, seq2));
  }
  {
    Seq seq1{'a'};
    Seq seq2{'a'};
    EXPECT_EQ(Seq{'a'}, LongestCommonSubsequence(seq1, seq2));
  }
  {
    Seq seq1{'a', 'b'};
    Seq seq2{'a'};
    EXPECT_EQ(Seq{'a'}, LongestCommonSubsequence(seq1, seq2));
  }
  {
    Seq seq1{'a', 'b'};
    Seq seq2{'b', 'a'};
    EXPECT_TRUE(LongestCommonSubsequence(seq1, seq2) == Seq{'a'} ||
                LongestCommonSubsequence(seq1, seq2) == Seq{'b'});
  }
  {
    Seq seq1{'a', 'b', 'c', 'd'};
    Seq seq2{};
    EXPECT_EQ(kEmpty, LongestCommonSubsequence(seq1, seq2));
  }
  {
    Seq seq1{'a', 'b', 'c', 'd'};
    Seq seq2{'1', 'a', 'b', 'd'};
    Seq lcs{'a', 'b', 'd'};
    EXPECT_EQ(lcs, LongestCommonSubsequence(seq1, seq2));
  }
  {
    Seq seq1{'a', 'b', 'c', 'd'};
    Seq seq2{'b', 'a', 'c'};
    Seq lcs1{'a', 'c'};
    Seq lcs2{'b', 'c'};
    EXPECT_TRUE(LongestCommonSubsequence(seq1, seq2) == lcs1 ||
                LongestCommonSubsequence(seq1, seq2) == lcs2);
  }
  {
    Seq seq1{'a', 'b', 'c', 'd'};
    Seq seq2{'1', 'b', '2', 'd', '1'};
    Seq lcs{'b', 'd'};
    EXPECT_EQ(lcs, LongestCommonSubsequence(seq1, seq2));
  }
  {
    Seq seq1{'a', 'b', 'c', 'd'};
    Seq seq2{'a', 'd'};
    Seq lcs{'a', 'd'};
    EXPECT_EQ(lcs, LongestCommonSubsequence(seq1, seq2));
  }
  {
    Seq seq1{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    Seq seq2{'g', 'a', 'b', '1', 'd', '2', '3', 'h', '4'};
    Seq lcs{'a', 'b', 'd', 'h'};
    EXPECT_EQ(lcs, LongestCommonSubsequence(seq1, seq2));
  }
}

TEST_F(HTMLSlotElementTest, TableSizeLimit) {
  Seq seq1;
  // If we use kTableSize here, it should hit DCHECK().
  std::fill_n(std::back_inserter(seq1), kTableSize - 1, 'a');
  Seq seq2;
  std::fill_n(std::back_inserter(seq2), kTableSize - 1, 'a');
  Seq lcs;
  std::fill_n(std::back_inserter(lcs), kTableSize - 1, 'a');
  EXPECT_EQ(lcs, LongestCommonSubsequence(seq1, seq2));
}

}  // namespace blink
