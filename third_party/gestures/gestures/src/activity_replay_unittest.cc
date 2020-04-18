// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "gestures/include/activity_replay.h"
#include "gestures/include/command_line.h"
#include "gestures/include/file_util.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/logging_filter_interpreter.h"
#include "gestures/include/gestures.h"
#include "gestures/include/string_util.h"

using std::string;

namespace gestures {

class ActivityReplayTest : public ::testing::Test {};

// This test reads a log file and replays it. This test should be enabled for a
// hands-on debugging session.

TEST(ActivityReplayTest, DISABLED_SimpleTest) {
  CommandLine* cl = CommandLine::ForCurrentProcess();
  GestureInterpreter* c_interpreter = NewGestureInterpreter();
  c_interpreter->Initialize();

  Interpreter* interpreter = c_interpreter->interpreter();
  PropRegistry* prop_reg = c_interpreter->prop_reg();
  {
    MetricsProperties mprops(prop_reg);

    string log_contents;
    ASSERT_TRUE(ReadFileToString(cl->GetSwitchValueASCII("in").c_str(),
                                 &log_contents));

    ActivityReplay replay(prop_reg);
    std::vector<string> honor_props;
    if (cl->GetSwitchValueASCII("only_honor")[0])
      SplitString(cl->GetSwitchValueASCII("only_honor"),
                        ',', &honor_props);
    std::set<string> honor_props_set(honor_props.begin(), honor_props.end());
    replay.Parse(log_contents, honor_props_set);
    replay.Replay(interpreter, &mprops);

    // Dump the new log
    const string kOutSwitchName = "outfile";
    if (cl->HasSwitch(kOutSwitchName))
      static_cast<LoggingFilterInterpreter*>(interpreter)->Dump(
          cl->GetSwitchValueASCII(kOutSwitchName).c_str());
  }

  DeleteGestureInterpreter(c_interpreter);
}

}  // namespace gestures
