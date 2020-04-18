// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/accessibility/inspect/ax_event_server.h"

#include <string>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_task_environment.h"

namespace content {

static void OnEvent(std::string event) {
  printf("Event %s\n", event.c_str());
}

AXEventServer::AXEventServer(int pid)
    : recorder_(AccessibilityEventRecorder::Create(nullptr, pid)) {
  printf("Events for process id: %d\n", pid);

  base::test::ScopedTaskEnvironment scoped_task_environment(
      base::test::ScopedTaskEnvironment::MainThreadType::UI);

  recorder_->ListenToEvents(base::BindRepeating(OnEvent));

  base::RunLoop run_loop;
  run_loop.Run();
}

AXEventServer::~AXEventServer() {
  delete recorder_.release();
}

}  // namespace content
