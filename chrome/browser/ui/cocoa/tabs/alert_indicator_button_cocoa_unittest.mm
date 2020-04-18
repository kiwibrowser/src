// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/alert_indicator_button_cocoa.h"

#include <string>

#include "base/command_line.h"
#include "base/mac/scoped_nsobject.h"
#include "base/test/scoped_task_environment.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/common/chrome_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

// A simple target to confirm an action was invoked.
@interface AlertIndicatorButtonTestTarget : NSObject {
 @private
  int count_;
}
@property(readonly, nonatomic) int count;
- (void)incrementCount:(id)sender;
@end

@implementation AlertIndicatorButtonTestTarget
@synthesize count = count_;
- (void)incrementCount:(id)sender {
  ++count_;
}
@end

namespace {

class AlertIndicatorButtonTestCocoa : public CocoaTest {
 public:
  AlertIndicatorButtonTestCocoa()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        std::string("--") + switches::kEnableTabAudioMuting);

    // Create the AlertIndicatorButton and add it to a view.
    button_.reset([[AlertIndicatorButton alloc] init]);
    EXPECT_TRUE(button_ != nil);
    [[test_window() contentView] addSubview:button_.get()];

    // Initially the button is disabled and showing no indicator.
    EXPECT_EQ(TabAlertState::NONE, [button_ showingAlertState]);
    EXPECT_FALSE([button_ isEnabled]);

    // Register target to be notified of clicks.
    base::scoped_nsobject<AlertIndicatorButtonTestTarget> clickTarget(
        [[AlertIndicatorButtonTestTarget alloc] init]);
    EXPECT_EQ(0, [clickTarget count]);
    [button_ setClickTarget:clickTarget withAction:@selector(incrementCount:)];

    // Transition to audio indicator mode, and expect button is enabled.
    [button_ transitionToAlertState:TabAlertState::AUDIO_PLAYING];
    EXPECT_EQ(TabAlertState::AUDIO_PLAYING, [button_ showingAlertState]);
    EXPECT_TRUE([button_ isEnabled]);

    // Click, and expect one click notification.
    EXPECT_EQ(0, [clickTarget count]);
    [button_ performClick:button_];
    EXPECT_EQ(1, [clickTarget count]);

    // Transition to audio muting mode, and expect button is still enabled.  A
    // click should result in another click notification.
    [button_ transitionToAlertState:TabAlertState::AUDIO_MUTING];
    EXPECT_EQ(TabAlertState::AUDIO_MUTING, [button_ showingAlertState]);
    EXPECT_TRUE([button_ isEnabled]);
    [button_ performClick:button_];
    EXPECT_EQ(2, [clickTarget count]);

    // Transition to capturing mode.  Now, the button is disabled since it
    // should only be drawing the indicator icon (i.e., there is nothing to
    // mute).  A click should NOT result in another click notification.
    [button_ transitionToAlertState:TabAlertState::TAB_CAPTURING];
    EXPECT_EQ(TabAlertState::TAB_CAPTURING, [button_ showingAlertState]);
    EXPECT_FALSE([button_ isEnabled]);
    [button_ performClick:button_];
    EXPECT_EQ(2, [clickTarget count]);
  }

  base::scoped_nsobject<AlertIndicatorButton> button_;

  // Needed for gfx::Animation.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_VIEW(AlertIndicatorButtonTestCocoa, button_)

}  // namespace
