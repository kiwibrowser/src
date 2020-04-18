// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_learn_more_item.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using ContentSuggestionsLearnMoreItemTest = PlatformTest;

TEST_F(ContentSuggestionsLearnMoreItemTest, CellClass) {
  // Setup.
  ContentSuggestionsLearnMoreItem* item =
      [[ContentSuggestionsLearnMoreItem alloc] initWithType:0];

  // Action.
  ContentSuggestionsLearnMoreCell* cell = [[[item cellClass] alloc] init];

  // Test.
  EXPECT_EQ([ContentSuggestionsLearnMoreCell class], [cell class]);
}

TEST_F(ContentSuggestionsLearnMoreItemTest, Configure) {
  // Setup.
  ContentSuggestionsLearnMoreItem* item =
      [[ContentSuggestionsLearnMoreItem alloc] initWithType:0];
  id cell = OCMClassMock([ContentSuggestionsLearnMoreCell class]);
  OCMExpect([cell setText:item.text]);

  // Action.
  [item configureCell:cell];

  // Test.
  ASSERT_OCMOCK_VERIFY(cell);
}
}
