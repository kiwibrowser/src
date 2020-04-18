// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/public/cpp/perfetto/task_runner.h"

#include <memory>
#include <utility>

#include "base/threading/sequenced_task_runner_handle.h"

namespace tracing {

PerfettoTaskRunner::PerfettoTaskRunner(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)) {}

PerfettoTaskRunner::~PerfettoTaskRunner() = default;

void PerfettoTaskRunner::PostTask(std::function<void()> task) {
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce([](std::function<void()> task) { task(); }, task));
}

void PerfettoTaskRunner::PostDelayedTask(std::function<void()> task,
                                         uint32_t delay_ms) {
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce([](std::function<void()> task) { task(); }, task),
      base::TimeDelta::FromMilliseconds(delay_ms));
}

void PerfettoTaskRunner::AddFileDescriptorWatch(int fd, std::function<void()>) {
  NOTREACHED();
}

void PerfettoTaskRunner::RemoveFileDescriptorWatch(int fd) {
  NOTREACHED();
}

void PerfettoTaskRunner::ResetTaskRunnerForTesting(
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  task_runner_ = std::move(task_runner);
}

}  // namespace tracing
