// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_utils.h"

#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class TabSwitcherUtilsTest : public PlatformTest {
 protected:
  // Checks that the computed updates/deletions/insertions to go from |initial|
  //  to |final| correctly match the expected updates/deletions/insertions.
  void TestLevenshtein(std::vector<size_t> const& initial,
                       std::vector<size_t> const& final,
                       std::vector<size_t> const& expectedSubstitutions,
                       std::vector<size_t> const& expectedDeletions,
                       std::vector<size_t> const& expectedInsertions) {
    std::vector<size_t> substitutions;
    std::vector<size_t> deletions;
    std::vector<size_t> insertions;
    TabSwitcherMinimalReplacementOperations(initial, final, &substitutions,
                                            &deletions, &insertions);
    EXPECT_EQ(substitutions, expectedSubstitutions);
    EXPECT_EQ(deletions, expectedDeletions);
    EXPECT_EQ(insertions, expectedInsertions);
  }
};

TEST_F(TabSwitcherUtilsTest, TestLevenshteinEmptyVectors) {
  std::vector<size_t> initial = {};
  std::vector<size_t> final = {};
  std::vector<size_t> expectedSubstitutions = {};
  std::vector<size_t> expectedDeletions = {};
  std::vector<size_t> expectedInsertions = {};
  TestLevenshtein(initial, final, expectedSubstitutions, expectedDeletions,
                  expectedInsertions);
}

TEST_F(TabSwitcherUtilsTest, TestLevenshteinNoChange) {
  std::vector<size_t> initial = {1, 2, 3};
  std::vector<size_t> final = {1, 2, 3};
  std::vector<size_t> expectedSubstitutions = {};
  std::vector<size_t> expectedDeletions = {};
  std::vector<size_t> expectedInsertions = {};
  TestLevenshtein(initial, final, expectedSubstitutions, expectedDeletions,
                  expectedInsertions);
}

TEST_F(TabSwitcherUtilsTest, TestLevenshteinInsertions) {
  std::vector<size_t> initial = {1};
  std::vector<size_t> final = {0, 1, 2};
  std::vector<size_t> expectedSubstitutions = {};
  std::vector<size_t> expectedDeletions = {};
  std::vector<size_t> expectedInsertions = {0, 2};
  TestLevenshtein(initial, final, expectedSubstitutions, expectedDeletions,
                  expectedInsertions);
}

TEST_F(TabSwitcherUtilsTest, TestLevenshteinDeletions) {
  std::vector<size_t> initial = {0, 1, 2, 3, 4, 5};
  std::vector<size_t> final = {0, 2, 3, 5};
  std::vector<size_t> expectedSubstitutions = {};
  std::vector<size_t> expectedDeletions = {1, 4};
  std::vector<size_t> expectedInsertions = {};
  TestLevenshtein(initial, final, expectedSubstitutions, expectedDeletions,
                  expectedInsertions);
}

TEST_F(TabSwitcherUtilsTest, TestLevenshteinFromAndToEmptyVectors) {
  std::vector<size_t> empty = {};
  std::vector<size_t> nonEmpty = {0, 1, 2};
  std::vector<size_t> expectedSubstitutions = {};
  std::vector<size_t> expectedDeletions = {};
  std::vector<size_t> expectedInsertions = {0, 1, 2};
  TestLevenshtein(empty, nonEmpty, expectedSubstitutions, expectedDeletions,
                  expectedInsertions);

  // Tests the reverse transformation.
  TestLevenshtein(nonEmpty, empty, expectedSubstitutions, expectedInsertions,
                  expectedDeletions);
}

TEST_F(TabSwitcherUtilsTest, TestLevenshteinSubstitutions) {
  std::vector<size_t> initial = {0, 1, 2, 3, 4, 5, 6};
  std::vector<size_t> final = {0, 1, 2, 9, 8, 5, 6};
  std::vector<size_t> expectedSubstitutions = {3, 4};
  std::vector<size_t> expectedDeletions = {};
  std::vector<size_t> expectedInsertions = {};
  TestLevenshtein(initial, final, expectedSubstitutions, expectedDeletions,
                  expectedInsertions);
}

TEST_F(TabSwitcherUtilsTest, TestLevenshteinInsertionAndDeletions) {
  std::vector<size_t> initial = {0, 1, 2, 3, 4, 5, 6, 7};
  std::vector<size_t> final = {0, 1, 8, 9, 2, 3, 4, 7};
  std::vector<size_t> expectedSubstitutions = {};
  std::vector<size_t> expectedDeletions = {5, 6};
  std::vector<size_t> expectedInsertions = {2, 3};
  TestLevenshtein(initial, final, expectedSubstitutions, expectedDeletions,
                  expectedInsertions);

  // Tests the reverse transformation.
  TestLevenshtein(final, initial, expectedSubstitutions, expectedInsertions,
                  expectedDeletions);
}

TEST_F(TabSwitcherUtilsTest, TestLevenshteinInsertionAndSubstitutions) {
  std::vector<size_t> initial = {0, 1, 2, 3, 4, 5, 6, 7};
  std::vector<size_t> final = {0, 1, 2, 99, 98, 3, 4, 97, 96, 7};
  std::vector<size_t> expectedSubstitutions = {5, 6};
  std::vector<size_t> expectedDeletions = {};
  std::vector<size_t> expectedInsertions = {3, 4};
  TestLevenshtein(initial, final, expectedSubstitutions, expectedDeletions,
                  expectedInsertions);
}

TEST_F(TabSwitcherUtilsTest,
       TestLevenshteinPreferInsertionsAndDeletionsOverSubstitutions) {
  std::vector<size_t> initial = {0, 1};
  std::vector<size_t> final = {1, 2};
  std::vector<size_t> expectedSubstitutions = {};
  std::vector<size_t> expectedDeletions = {0};
  std::vector<size_t> expectedInsertions = {1};
  TestLevenshtein(initial, final, expectedSubstitutions, expectedDeletions,
                  expectedInsertions);
}
