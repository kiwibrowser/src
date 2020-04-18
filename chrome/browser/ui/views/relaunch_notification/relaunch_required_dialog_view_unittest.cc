// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/relaunch_notification/relaunch_required_dialog_view.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(RelaunchRequiredDialogViewTest, ComputeDeadlineDelta) {
  // A tiny bit over three days: round to three days.
  EXPECT_EQ(
      RelaunchRequiredDialogView::ComputeDeadlineDelta(
          base::TimeDelta::FromDays(3) + base::TimeDelta::FromSecondsD(0.1)),
      base::TimeDelta::FromDays(3));
  // Exactly three days: three days.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeDeadlineDelta(
                base::TimeDelta::FromDays(3)),
            base::TimeDelta::FromDays(3));
  // Almost three days: round to three days.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeDeadlineDelta(
                base::TimeDelta::FromDays(3) - base::TimeDelta::FromHours(1)),
            base::TimeDelta::FromDays(3));
  // Just shy of two days: round up to two days.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeDeadlineDelta(
                base::TimeDelta::FromDays(2) - base::TimeDelta::FromMinutes(1)),
            base::TimeDelta::FromDays(2));
  // A bit over 47 hours: round down to 47 hours.
  EXPECT_EQ(
      RelaunchRequiredDialogView::ComputeDeadlineDelta(
          base::TimeDelta::FromDays(2) - base::TimeDelta::FromMinutes(45)),
      base::TimeDelta::FromHours(47));
  // Less than one and a half hours: round down to one hour.
  EXPECT_EQ(
      RelaunchRequiredDialogView::ComputeDeadlineDelta(
          base::TimeDelta::FromHours(1) + base::TimeDelta::FromMinutes(23)),
      base::TimeDelta::FromHours(1));
  // Exactly one hour: one hour.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeDeadlineDelta(
                base::TimeDelta::FromHours(1)),
            base::TimeDelta::FromHours(1));
  // A bit over three minutes: round down to three minutes.
  EXPECT_EQ(
      RelaunchRequiredDialogView::ComputeDeadlineDelta(
          base::TimeDelta::FromMinutes(3) + base::TimeDelta::FromSeconds(12)),
      base::TimeDelta::FromMinutes(3));
  // Exactly two minutes: two minutes.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeDeadlineDelta(
                base::TimeDelta::FromMinutes(2)),
            base::TimeDelta::FromMinutes(2));
  // Nearly one minute: one minute.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeDeadlineDelta(
                base::TimeDelta::FromMilliseconds(60 * 1000 - 250)),
            base::TimeDelta::FromMinutes(1));
  // Just over two seconds: two seconds.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeDeadlineDelta(
                base::TimeDelta::FromMilliseconds(2 * 1000 + 250)),
            base::TimeDelta::FromSeconds(2));
  // Exactly one second: one second.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeDeadlineDelta(
                base::TimeDelta::FromSeconds(1)),
            base::TimeDelta::FromSeconds(1));
  // Next to nothing: zero.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeDeadlineDelta(
                base::TimeDelta::FromMilliseconds(250)),
            base::TimeDelta());
}

TEST(RelaunchRequiredDialogViewTest, ComputeNextRefreshDelta) {
  // Over three days: align to the next smallest day boundary.
  EXPECT_EQ(
      RelaunchRequiredDialogView::ComputeNextRefreshDelta(
          base::TimeDelta::FromDays(3) + base::TimeDelta::FromSecondsD(0.1)),
      base::TimeDelta::FromDays(1) + base::TimeDelta::FromSecondsD(0.1));
  // Exactly three days: align to the next smallest day boundary.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeNextRefreshDelta(
                base::TimeDelta::FromDays(3)),
            base::TimeDelta::FromDays(1));
  // Almost three days: align to two days.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeNextRefreshDelta(
                base::TimeDelta::FromDays(3) - base::TimeDelta::FromHours(1)),
            base::TimeDelta::FromHours(23));
  // A bit over two days: align to 47 hours.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeNextRefreshDelta(
                base::TimeDelta::FromDays(2) + base::TimeDelta::FromHours(1)),
            base::TimeDelta::FromHours(2));
  // Exactly two days: align to 47 hours.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeNextRefreshDelta(
                base::TimeDelta::FromDays(2)),
            base::TimeDelta::FromHours(1));
  // Less than one and a half hours: align to 59 minutes.
  EXPECT_EQ(
      RelaunchRequiredDialogView::ComputeNextRefreshDelta(
          base::TimeDelta::FromHours(1) + base::TimeDelta::FromMinutes(23)),
      base::TimeDelta::FromMinutes(24));
  // Exactly one hour: align to 59 minutes.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeNextRefreshDelta(
                base::TimeDelta::FromHours(1)),
            base::TimeDelta::FromMinutes(1));
  // Between one and two minutes: align to 59 seconds.
  EXPECT_EQ(
      RelaunchRequiredDialogView::ComputeNextRefreshDelta(
          base::TimeDelta::FromMinutes(1) + base::TimeDelta::FromSeconds(12)),
      base::TimeDelta::FromSeconds(13));
  // Exactly one minute: align to 59 seconds.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeNextRefreshDelta(
                base::TimeDelta::FromMinutes(1)),
            base::TimeDelta::FromSeconds(1));
  // One and a half seconds: align to 1 second.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeNextRefreshDelta(
                base::TimeDelta::FromSecondsD(1.5)),
            base::TimeDelta::FromSecondsD(0.5));
  // Just over one second: align to 0 seconds.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeNextRefreshDelta(
                base::TimeDelta::FromSecondsD(1.1)),
            base::TimeDelta::FromSecondsD(1.1));
  // Exactly one second: align to 0 seconds.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeNextRefreshDelta(
                base::TimeDelta::FromSeconds(1)),
            base::TimeDelta::FromSeconds(1));
  // Less than one second: align to 0 seconds.
  EXPECT_EQ(RelaunchRequiredDialogView::ComputeNextRefreshDelta(
                base::TimeDelta::FromSecondsD(0.1)),
            base::TimeDelta::FromSecondsD(0.1));
}
