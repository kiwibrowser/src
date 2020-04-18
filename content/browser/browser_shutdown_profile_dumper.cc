// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/browser_shutdown_profile_dumper.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_impl.h"
#include "components/tracing/common/tracing_switches.h"

namespace content {

BrowserShutdownProfileDumper::BrowserShutdownProfileDumper(
    const base::FilePath& dump_file_name)
    : dump_file_name_(dump_file_name), blocks_(0), dump_file_(nullptr) {}

BrowserShutdownProfileDumper::~BrowserShutdownProfileDumper() {
  WriteTracesToDisc();
}

static float GetTraceBufferPercentFull() {
  base::trace_event::TraceLogStatus status =
      base::trace_event::TraceLog::GetInstance()->GetStatus();
  return 100 * static_cast<float>(static_cast<double>(status.event_count) /
                                  status.event_capacity);
}

void BrowserShutdownProfileDumper::WriteTracesToDisc() {
  // Note: I have seen a usage of 0.000xx% when dumping - which fits easily.
  // Since the tracer stops when the trace buffer is filled, we'd rather save
  // what we have than nothing since we might see from the amount of events
  // that caused the problem.
  DVLOG(1) << "Flushing shutdown traces to disc. The buffer is "
           << GetTraceBufferPercentFull() << "% full.";
  DCHECK(!dump_file_);
  dump_file_ = base::OpenFile(dump_file_name_, "w+");
  if (!IsFileValid()) {
    LOG(ERROR) << "Failed to open performance trace file: "
               << dump_file_name_.value();
    return;
  }
  WriteString("{\"traceEvents\":");
  WriteString("[");

  // TraceLog::Flush() requires the calling thread to have a message loop.
  // As the message loop of the current thread may have quit, start another
  // thread for flushing the trace.
  base::WaitableEvent flush_complete_event(
      base::WaitableEvent::ResetPolicy::AUTOMATIC,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  base::Thread flush_thread("browser_shutdown_trace_event_flush");
  flush_thread.Start();
  flush_thread.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&BrowserShutdownProfileDumper::EndTraceAndFlush,
                                base::Unretained(this),
                                base::Unretained(&flush_complete_event)));

  bool original_wait_allowed = base::ThreadRestrictions::SetWaitAllowed(true);
  flush_complete_event.Wait();
  base::ThreadRestrictions::SetWaitAllowed(original_wait_allowed);
}

void BrowserShutdownProfileDumper::EndTraceAndFlush(
    base::WaitableEvent* flush_complete_event) {
  base::trace_event::TraceLog::GetInstance()->SetDisabled();
  base::trace_event::TraceLog::GetInstance()->Flush(
      base::Bind(&BrowserShutdownProfileDumper::WriteTraceDataCollected,
                 base::Unretained(this),
                 base::Unretained(flush_complete_event)));
}

// static
base::FilePath BrowserShutdownProfileDumper::GetShutdownProfileFileName() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  base::FilePath trace_file =
      command_line.GetSwitchValuePath(switches::kTraceShutdownFile);

  if (!trace_file.empty())
    return trace_file;

  // Default to saving the startup trace into the current dir.
  return base::FilePath().AppendASCII("chrometrace.log");
}

void BrowserShutdownProfileDumper::WriteTraceDataCollected(
    base::WaitableEvent* flush_complete_event,
    const scoped_refptr<base::RefCountedString>& events_str,
    bool has_more_events) {
  if (!IsFileValid()) {
    flush_complete_event->Signal();
    return;
  }
  if (blocks_) {
    // Blocks are not comma separated. Beginning with the second block we
    // start therefore to add one in front of the previous block.
    WriteString(",");
  }
  ++blocks_;
  WriteString(events_str->data());

  if (!has_more_events) {
    WriteString("]");
    WriteString("}");
    CloseFile();
    flush_complete_event->Signal();
  }
}

bool BrowserShutdownProfileDumper::IsFileValid() {
  return dump_file_ && (ferror(dump_file_) == 0);
}

void BrowserShutdownProfileDumper::WriteString(const std::string& string) {
  WriteChars(string.data(), string.size());
}

void BrowserShutdownProfileDumper::WriteChars(const char* chars, size_t size) {
  if (!IsFileValid())
    return;

  size_t written = fwrite(chars, 1, size, dump_file_);
  if (written != size) {
    LOG(ERROR) << "Error " << ferror(dump_file_)
               << " in fwrite() to trace file '" << dump_file_name_.value()
               << "'";
    CloseFile();
  }
}

void BrowserShutdownProfileDumper::CloseFile() {
  if (!dump_file_)
    return;
  base::CloseFile(dump_file_);
  dump_file_ = nullptr;
}

}  // namespace content
