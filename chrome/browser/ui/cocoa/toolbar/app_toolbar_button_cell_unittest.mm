// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/toolbar/app_toolbar_button_cell.h"

#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"

@interface TestAppToolbarButton : NSButton
@end

@implementation TestAppToolbarButton

+ (Class)cellClass {
  return [AppToolbarButtonCell class];
}

@end

class AppToolbarButtonCellTest : public CocoaTest {
 protected:
  AppToolbarButtonCellTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {
    base::scoped_nsobject<NSButton> button([[TestAppToolbarButton alloc]
        initWithFrame:NSMakeRect(0, 0, 29, 29)]);
    button_ = button;
    [[test_window() contentView] addSubview:button_];
  }

  NSButton* button_;
  base::scoped_nsobject<AppToolbarButtonCell> cell_;

  // Needed for gfx::Animation.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppToolbarButtonCellTest);
};

TEST_VIEW(AppToolbarButtonCellTest, button_)
