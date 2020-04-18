// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_TRACING_SYSTEM_TRACER_H_
#define CHROMECAST_TRACING_SYSTEM_TRACER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/message_loop/message_pump_libevent.h"

namespace chromecast {

class SystemTracer {
 public:
  SystemTracer();
  ~SystemTracer();

  enum class Status {
    OK,
    KEEP_GOING,
    FAIL,
  };

  using StartTracingCallback = base::OnceCallback<void(Status)>;
  using StopTracingCallback =
      base::RepeatingCallback<void(Status status, std::string trace_data)>;

  // Start system tracing for categories in |categories| (comma separated).
  void StartTracing(base::StringPiece categories,
                    StartTracingCallback callback);

  // Stop system tracing.
  //
  // This will call |callback| on the current thread with the trace data. If
  // |status| is Status::KEEP_GOING, another call will be made with additional
  // data.
  void StopTracing(const StopTracingCallback& callback);

 private:
  enum class State {
    INITIAL,   // Not yet started.
    STARTING,  // Sent start message, waiting for ack.
    TRACING,   // Tracing, not yet requested stop.
    READING,   // Trace stopped, reading output.
    FINISHED,  // All done.
  };

  void ReceiveStartAckAndTracePipe();
  void ReceiveTraceData();
  void FailStartTracing();
  void FailStopTracing();
  void SendPartialTraceData();
  void FinishTracing();
  void Cleanup();

  // Current state of tracing attempt.
  State state_ = State::INITIAL;

  // Unix socket connection to tracing daemon.
  base::ScopedFD connection_fd_;
  std::unique_ptr<base::FileDescriptorWatcher::Controller> connection_watcher_;

  // Pipe for trace data.
  base::ScopedFD trace_pipe_fd_;
  std::unique_ptr<base::FileDescriptorWatcher::Controller> trace_pipe_watcher_;

  // Read buffer (of size kBufferSize).
  std::unique_ptr<char[]> buffer_;

  // Callbacks for StartTracing() and StopTracing().
  StartTracingCallback start_tracing_callback_;
  StopTracingCallback stop_tracing_callback_;

  // Trace data.
  std::string trace_data_;

  DISALLOW_COPY_AND_ASSIGN(SystemTracer);
};

}  // namespace chromecast

#endif  // CHROMECAST_TRACING_SYSTEM_TRACER_H_
