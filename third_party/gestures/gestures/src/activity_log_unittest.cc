// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include <gtest/gtest.h>

#include "gestures/include/activity_log.h"
#include "gestures/include/macros.h"
#include "gestures/include/prop_registry.h"

using std::string;

namespace gestures {

class ActivityLogTest : public ::testing::Test {};

TEST(ActivityLogTest, SimpleTest) {
  PropRegistry prop_reg;
  BoolProperty true_prop(&prop_reg, "true prop", true);
  BoolProperty false_prop(&prop_reg, "false prop", false);
  DoubleProperty double_prop(&prop_reg, "double prop", 77.33);
  IntProperty int_prop(&prop_reg, "int prop", 816);
  ShortProperty short_prop(&prop_reg, "short prop", -998);
  StringProperty string_prop(&prop_reg, "string prop", "foobarstr");

  ActivityLog log(&prop_reg);
  EXPECT_TRUE(strstr(log.Encode().c_str(), "true"));
  EXPECT_TRUE(strstr(log.Encode().c_str(), "false"));
  EXPECT_TRUE(strstr(log.Encode().c_str(), "77.33"));
  EXPECT_TRUE(strstr(log.Encode().c_str(), "816"));
  EXPECT_TRUE(strstr(log.Encode().c_str(), "-998"));
  EXPECT_TRUE(strstr(log.Encode().c_str(), "foobarstr"));

  HardwareProperties hwprops = {
    6011,  // left edge
    6012,  // top edge
    6013,  // right edge
    6014,  // bottom edge
    6015,  // x pixels/TP width
    6016,  // y pixels/TP height
    6017,  // x screen DPI
    6018,  // y screen DPI
    6019,  // orientation minimum
    6020,  // orientation maximum
    6021,  // max fingers
    6022,  // max touch
    1,  // t5r2
    0,  // semi-mt
    1,  // is button pad,
    0   // has wheel
  };

  log.SetHardwareProperties(hwprops);

  const char* expected_strings[] = {
    "6011", "6012", "6013", "6014", "6015", "6016",
    "6017", "6018", "6019", "6020", "6021", "6022"
  };
  string hwprops_log = log.Encode();
  for (size_t i = 0; i < arraysize(expected_strings); i++)
    EXPECT_TRUE(strstr(hwprops_log.c_str(), expected_strings[i]));

  EXPECT_EQ(0, log.size());
  EXPECT_GT(log.MaxSize(), 10);

  FingerState fs = { 0.0, 0.0, 0.0, 0.0, 9.0, 0.0, 3.0, 4.0, 22, 0 };
  HardwareState hs = { 1.0, 0, 1, 1, &fs, 0, 0, 0, 0 };
  log.LogHardwareState(hs);
  EXPECT_EQ(1, log.size());
  EXPECT_TRUE(strstr(log.Encode().c_str(), "22"));
  ActivityLog::Entry* entry = log.GetEntry(0);
  EXPECT_EQ(ActivityLog::kHardwareState, entry->type);

  log.LogTimerCallback(234.5);
  EXPECT_EQ(2, log.size());
  EXPECT_TRUE(strstr(log.Encode().c_str(), "234.5"));
  entry = log.GetEntry(1);
  EXPECT_EQ(ActivityLog::kTimerCallback, entry->type);

  log.LogCallbackRequest(90210);
  EXPECT_EQ(3, log.size());
  EXPECT_TRUE(strstr(log.Encode().c_str(), "90210"));
  entry = log.GetEntry(2);
  EXPECT_EQ(ActivityLog::kCallbackRequest, entry->type);

  Gesture null;
  Gesture move(kGestureMove, 1.0, 2.0, 773, 4.0);
  Gesture scroll(kGestureScroll, 1.0, 2.0, 312, 4.0);
  Gesture buttons(kGestureButtonsChange, 1.0, 847, 3, 4);
  Gesture contact_initiated;
  contact_initiated.type = kGestureTypeContactInitiated;

  Gesture* gs[] = { &null, &move, &scroll, &buttons, &contact_initiated };
  const char* test_strs[] = { "null", "773", "312", "847", "nitiated" };

  ASSERT_EQ(arraysize(gs), arraysize(test_strs));
  for (size_t i = 0; i < arraysize(gs); ++i) {
    log.LogGesture(*gs[i]);
    EXPECT_TRUE(strstr(log.Encode().c_str(), test_strs[i])) << "i=" << i;
    entry = log.GetEntry(log.size() - 1);
    EXPECT_EQ(ActivityLog::kGesture, entry->type) << "i=" << i;
  }

  log.Clear();
  EXPECT_EQ(0, log.size());
}

TEST(ActivityLogTest, WrapAroundTest) {
  ActivityLog log(NULL);
  // overfill the buffer
  const size_t fill_size = (ActivityLog::kBufferSize * 3) / 2;
  for (size_t i = 0; i < fill_size; i++)
    log.LogCallbackRequest(static_cast<stime_t>(i));
  const string::size_type prefix_length = 100;
  string first_prefix = log.Encode().substr(0, prefix_length);
  log.LogCallbackRequest(static_cast<stime_t>(fill_size));
  string second_prefix = log.Encode().substr(0, prefix_length);
  EXPECT_NE(first_prefix, second_prefix);
}

TEST(ActivityLogTest, VersionTest) {
  ActivityLog log(NULL);
  string thelog = log.Encode();
  EXPECT_TRUE(thelog.find(VCSID) != string::npos);
}

}  // namespace gestures
