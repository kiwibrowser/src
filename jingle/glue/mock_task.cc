// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/glue/mock_task.h"

namespace jingle_glue {

MockTask::MockTask(TaskParent* parent) : rtc::Task(parent) {}

MockTask::~MockTask() {}

}  // namespace jingle_glue
