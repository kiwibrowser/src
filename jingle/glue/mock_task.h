// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A mock of rtc::Task.

#ifndef JINGLE_GLUE_MOCK_TASK_H_
#define JINGLE_GLUE_MOCK_TASK_H_

#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/libjingle_xmpp/task_runner/task.h"

namespace jingle_glue {

class MockTask : public rtc::Task {
 public:
  MockTask(TaskParent* parent);

  ~MockTask() override;

  MOCK_METHOD0(ProcessStart, int());
};

}  // namespace jingle_glue

#endif  // JINGLE_GLUE_MOCK_TASK_H_
