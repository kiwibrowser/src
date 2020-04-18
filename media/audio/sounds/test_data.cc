// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/sounds/test_data.h"

#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"

namespace media {

TestObserver::TestObserver(const base::Closure& quit)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      quit_(quit),
      num_play_requests_(0),
      num_stop_requests_(0),
      cursor_(0) {}

TestObserver::~TestObserver() = default;

void TestObserver::OnPlay() {
  ++num_play_requests_;
}

void TestObserver::OnStop(size_t cursor) {
  ++num_stop_requests_;
  cursor_ = cursor;
  task_runner_->PostTask(FROM_HERE, quit_);
}

}  // namespace media
