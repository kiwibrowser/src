// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_PUBLIC_CPP_PERFETTO_TASK_RUNNER_H_
#define SERVICES_TRACING_PUBLIC_CPP_PERFETTO_TASK_RUNNER_H_

#include "base/component_export.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "services/tracing/public/mojom/perfetto_service.mojom.h"
#include "third_party/perfetto/include/perfetto/base/task_runner.h"

namespace tracing {

// This wraps a base::TaskRunner implementation to be able
// to provide it to Perfetto.
class COMPONENT_EXPORT(TRACING_CPP) PerfettoTaskRunner
    : public perfetto::base::TaskRunner {
 public:
  explicit PerfettoTaskRunner(
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~PerfettoTaskRunner() override;

  // perfetto::base::TaskRunner implementation. Only called by
  // the Perfetto implementation itself.
  void PostTask(std::function<void()> task) override;
  void PostDelayedTask(std::function<void()> task, uint32_t delay_ms) override;

  // Not used in Chrome.
  void AddFileDescriptorWatch(int fd, std::function<void()>) override;
  void RemoveFileDescriptorWatch(int fd) override;

  base::SequencedTaskRunner* task_runner() { return task_runner_.get(); }

  // Tests will shut down all task runners in between runs, so we need
  // to re-create any static instances on each SetUp();
  void ResetTaskRunnerForTesting(
      scoped_refptr<base::SequencedTaskRunner> task_runner);

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(PerfettoTaskRunner);
};

}  // namespace tracing

#endif  // SERVICES_TRACING_PUBLIC_CPP_PERFETTO_TASK_RUNNER_H_
