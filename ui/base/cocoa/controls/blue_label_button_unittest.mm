// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/controls/blue_label_button.h"

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#import "testing/gtest_mac.h"
#import "ui/base/test/cocoa_helper.h"

namespace ui {
namespace test {

class BlueLabelButtonTest : public ui::CocoaTest {
 public:
  BlueLabelButtonTest() {}

  // ui::CocoaTest override:
  void SetUp() override;

 protected:
  base::scoped_nsobject<BlueLabelButton> blue_label_button_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BlueLabelButtonTest);
};

void BlueLabelButtonTest::SetUp() {
  blue_label_button_.reset([[BlueLabelButton alloc] initWithFrame:NSZeroRect]);
  [blue_label_button_ setTitle:@"Test Title"];
  [blue_label_button_ sizeToFit];
  ui::CocoaTest::SetUp();
  [[test_window() contentView] addSubview:blue_label_button_];
}

TEST_VIEW(BlueLabelButtonTest, blue_label_button_);

}  // namespace test
}  // namespace ui
