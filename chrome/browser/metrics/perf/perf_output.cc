// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/perf/perf_output.h"

#include "base/bind.h"
#include "base/task_scheduler/post_task.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"

PerfOutputCall::PerfOutputCall(base::TimeDelta duration,
                               const std::vector<std::string>& perf_args,
                               const DoneCallback& callback)
    : duration_(duration),
      perf_args_(perf_args),
      done_callback_(callback),
      weak_factory_(this) {
  DCHECK(thread_checker_.CalledOnValidThread());

  perf_data_pipe_reader_ =
      std::make_unique<chromeos::PipeReader>(base::CreateTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE}));

  base::ScopedFD pipe_write_end =
      perf_data_pipe_reader_->StartIO(base::BindOnce(
          &PerfOutputCall::OnIOComplete, weak_factory_.GetWeakPtr()));
  chromeos::DebugDaemonClient* client =
      chromeos::DBusThreadManager::Get()->GetDebugDaemonClient();
  client->GetPerfOutput(duration_, perf_args_, pipe_write_end.get(),
                        base::BindOnce(&PerfOutputCall::OnGetPerfOutput,
                                       weak_factory_.GetWeakPtr()));
}

PerfOutputCall::~PerfOutputCall() {}

void PerfOutputCall::OnIOComplete(base::Optional<std::string> result) {
  DCHECK(thread_checker_.CalledOnValidThread());
  perf_data_pipe_reader_.reset();
  done_callback_.Run(result.value_or(std::string()));
  // The callback may delete us, so it's hammertime: Can't touch |this|.
}

void PerfOutputCall::OnGetPerfOutput(bool success) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Signal pipe reader to shut down.
  if (!success && perf_data_pipe_reader_.get()) {
    perf_data_pipe_reader_.reset();
    done_callback_.Run(std::string());
  }
}
