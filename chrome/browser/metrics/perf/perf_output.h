// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_PERF_PERF_OUTPUT_H_
#define CHROME_BROWSER_METRICS_PERF_PERF_OUTPUT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "chromeos/dbus/pipe_reader.h"

// Class for handling getting output from perf over DBus. Manages the
// asynchronous DBus call and retrieving data from quipper over a pipe.
class PerfOutputCall {
 public:
  // Called once GetPerfOutput() is complete, or an error occurred.
  // The callback may delete this object.
  // The argument is one of:
  // - Output from "perf record", in PerfDataProto format, OR
  // - Output from "perf stat", in PerfStatProto format, OR
  // - The empty string if there was an error.
  using DoneCallback = base::Callback<void(const std::string& perf_stdout)>;

  PerfOutputCall(base::TimeDelta duration,
                 const std::vector<std::string>& perf_args,
                 const DoneCallback& callback);
  ~PerfOutputCall();

 private:
  // Internal callbacks.
  void OnIOComplete(base::Optional<std::string> data);
  void OnGetPerfOutput(bool success);

  // Used to capture perf data written to a pipe.
  std::unique_ptr<chromeos::PipeReader> perf_data_pipe_reader_;

  // Saved arguments.
  base::TimeDelta duration_;
  std::vector<std::string> perf_args_;
  DoneCallback done_callback_;

  base::ThreadChecker thread_checker_;

  // To pass around the "this" pointer across threads safely.
  base::WeakPtrFactory<PerfOutputCall> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PerfOutputCall);
};

#endif  // CHROME_BROWSER_METRICS_PERF_PERF_OUTPUT_H_
